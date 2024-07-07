#include "elf_parser.h"


SC_MODULE(RV_DECODER)
{
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_in<sc_uint<32>> instr;
    sc_out<sc_uint<5>> rs1;
    sc_out<sc_uint<5>> rs2;
    sc_out<sc_uint<5>> rd;
    sc_out<sc_uint<32>> imm_12_itype;
    sc_out<sc_uint<32>> imm_12_sbtype;
    sc_out<sc_uint<32>> imm_20_ujtype;
    sc_out<sc_uint<32>> selected_imm;

    void perform_decoding()
    {
        rs1 = instr.read().range(20, 15); // bits 15-20
        rs2 = instr.read().range(25, 21); // bits 21-25
        rd = instr.read().range(12, 7);   // bits 7-12
        imm_12_itype = instr.read().range(31,20);  // bits 31,20
        imm_12_sbtype = (instr.read().range(31,24) << 5) | instr.read().range(11,7);
        imm_20_ujtype = instr.read().range(31,12);
    }

    SC_CTOR(RV_DECODER)
    {
        SC_METHOD(perform_decoding);
        sensitive << clk.pos();
        sensitive << reset;
    }
};

SC_MODULE(Testbench)
{
    RV_DECODER *rv_dec;
    uint8_t *mem;
    sc_signal<bool> clk;
    sc_signal<bool> reset;
    sc_signal<sc_uint<32>> instruction;
    uint32_t pc_val = 0;
    sc_signal<sc_uint<32>> pc;
    sc_signal<sc_uint<5>> rs1;
    sc_signal<sc_uint<5>> rs2;
    sc_signal<sc_uint<5>> rd;
    sc_signal<sc_uint<32>> imm_12_itype;
    sc_signal<sc_uint<32>> imm_12_sbtype;
    sc_signal<sc_uint<32>> imm_20_ujtype;
    sc_signal<sc_uint<32>> selected_imm;
    uint32_t mem_size = 0;

    void generate_clock_pulse()
    {
        while (true)
        {
            clk.write(1);
            wait(5, SC_NS);
            clk.write(0);
            wait(5, SC_NS);
        }
    }

    void decode_instruction()
    {
        if (clk.posedge())
        {
            if (pc_val <= mem_size)
            {
                pc.write(pc_val);
                uint32_t *mem_addr = ((uint32_t *)mem);
                instruction.write(mem_addr[pc_val]);
                cout << "Timestamp: " << sc_time_stamp() << " pc_val " << pc_val << "| instr: " << instruction << " rs1: " << rs1 << " rs2: " << rs2 << " rd: " << rd
                     << ", imm_12_itype: " << imm_12_itype << ", imm_12_sbtype: " << imm_12_sbtype << ", imm_20_ujtype: " << imm_20_ujtype << ", selected_imm: " << selected_imm << endl;
                pc_val = pc_val + 4;
            }
        }
    }

    SC_CTOR(Testbench)
    {
        rv_dec = new RV_DECODER("rv_decoder");
        rv_dec->instr(instruction);
        rv_dec->clk(clk);
        rv_dec->reset(reset);
        rv_dec->rs1(rs1);
        rv_dec->rs2(rs2);
        rv_dec->rd(rd);
        rv_dec->imm_12_itype(imm_12_itype);
        rv_dec->imm_12_sbtype(imm_12_sbtype);
        rv_dec->imm_20_ujtype(imm_20_ujtype);
        rv_dec->selected_imm(selected_imm);
        SC_THREAD(generate_clock_pulse);
        SC_METHOD(decode_instruction);
        sensitive << clk.posedge_event();
    }
    void init_mem(uint8_t * memptr, uint32_t start_addr, uint32_t total_mem_size)
    {
        mem = memptr;
        mem_size = total_mem_size;
        pc_val = start_addr;
    }

    ~Testbench()
    {
        delete rv_dec;
    }
};

int sc_main(int argc, char *argv[])
{
    Testbench tb("tb");
    int msize = 1 << 22;
    uint8_t *mem;
    uint32_t total_mem_size = 0;
    uint32_t start_addr = 0;
    ELFParser elf_parser("elfs/linux.elf", &start_addr, &mem, &total_mem_size);
    tb.init_mem(mem, start_addr, total_mem_size);
    sc_start(200 * (int)total_mem_size, SC_NS);
    delete mem;
    return 0;
}