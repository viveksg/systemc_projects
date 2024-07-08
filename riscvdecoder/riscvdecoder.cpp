#include "elf_parser.h"
#include "isa.h"

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
    sc_out<sc_uint<32>> shift_amt;
    sc_out<sc_uint<32>> opcode_id;
    void perform_decoding()
    {
        rs1 = instr.read().range(20, 15);          // bits 15-20
        rs2 = instr.read().range(25, 21);          // bits 21-25
        rd = instr.read().range(12, 7);            // bits 7-12
        imm_12_itype = instr.read().range(31, 20); // bits 31,20
        imm_12_sbtype = (instr.read().range(31, 24) << 5) | instr.read().range(11, 7);
        imm_20_ujtype = instr.read().range(31, 12);
        shift_amt = instr.read().range(24, 20);
        for (int i = 0; i < MAX_INSTR; i++)
        {
            uint32_t opcode_val = instr.read();
            instr_def idef = instr_defs[i];
            if ((opcode_val & idef.instruction_mask) == idef.instruction_match)
            {
                opcode_id.write(i);
                switch (idef.immediate_type)
                {
                case IMM_TYPE_NONE:
                    selected_imm = 0;
                    break;
                case IMM_TYPE_IMM12:
                    selected_imm = imm_12_itype;
                    break;
                case IMM_TYPE_IMM20:
                    selected_imm = imm_20_ujtype;
                    break;
                case IMM_TYPE_SIMM:
                    selected_imm = imm_12_sbtype;
                    break;
                case IMM_TYPE_BIMM:
                    selected_imm = (imm_12_sbtype.read() << 1);
                    break;
                case IMM_TYPE_JIMM20:
                    selected_imm = (imm_20_ujtype.read() << 1);
                    break;
                default:
                    break;
                }
            }
        }
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
    sc_signal<sc_uint<32>> shift_amt;
    sc_signal<sc_uint<32>> opcode_id;
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
                cout << "Timestamp: " << sc_time_stamp() << " pc_val " << pc_val << "| instr: " << instruction <<": "<<inst_names[opcode_id.read()]<<", reg1: "<<gpr_names[rs1.read()] <<", reg2: "<<gpr_names[rs2.read()] <<", reg_rd: "<<gpr_names[rd.read()] <<", selected_imm: " << selected_imm << endl;
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
        rv_dec->shift_amt(shift_amt);
        rv_dec->opcode_id(opcode_id);
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