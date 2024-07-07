#include <systemc.h>
#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include "elf_parser.h"

bool compare_region(const region &region1, const region &region2)
{
    return region1.start_addr < region2.start_addr;
}

RegionManager::RegionManager()
{
    total_memory_size = 0;
}

void RegionManager::add_region(region mem_region)
{
    regions.push_back({mem_region.start_addr, mem_region.end_addr, mem_region.region_size, 0, mem_region.region_name});
}

void RegionManager::init_regions()
{
    sort(regions.begin(), regions.end(), compare_region);
    total_memory_size = 0;
    start_addr = regions[0].start_addr;
    for (int i = 0; i < regions.size(); i++)
    {
        regions[i].region_base = total_memory_size;
        total_memory_size += regions[i].region_size;
    }
}

uint32_t RegionManager::get_mem_address(uint32_t addr)
{
    for (auto region_val = regions.begin(); region_val != regions.end(); region_val++)
        if (region_val->start_addr <= addr && region_val->end_addr > addr)
        {
            return region_val->region_base + (addr - region_val->start_addr);
        }
    return -1;
}
void RegionManager::print_region_info()
{
    for (auto reg_val = regions.begin(); reg_val != regions.end(); reg_val++)
    {
        cout << reg_val->region_name << ", " << reg_val->start_addr << ", " << reg_val->end_addr << ", " << reg_val->region_size << ", " << reg_val->region_base << endl;
    }
    cout << "Total memory size: " << total_memory_size << endl;
}

uint32_t RegionManager::get_memory_size()
{
    return total_memory_size;
}

ELFParser::ELFParser(string file_location, uint32_t *start_addr, uint8_t **mem, uint32_t *total_memory_size)
{
    int elf_fd;
    Elf *elf_pointer;
    Elf_Kind elf_kind_val;
    Elf_Scn *elf_scn;
    Elf_Data *elf_data;
    Elf32_Shdr *elf_shdr;
    size_t section_heaher_index;
    uint32_t base_addr = 0xFFFFFFFFL;
    uint32_t mem_size = 0;
    vector<region> regions;
    RegionManager regmgr;
    // check if version is none
    if (elf_version(EV_CURRENT) == EV_NONE)
        return;
    // obtain filedescriptor to the elf_file
    if ((elf_fd = open((const char *)file_location.c_str(), O_RDONLY, 0)) < 0)
    {
        cout << "ELF FD " << elf_fd << endl;
        return;
    }
    // get pointer to the beginning
    if ((elf_pointer = elf_begin(elf_fd, ELF_C_READ, NULL)) == NULL)
        return;

    elf_kind_val = elf_kind(elf_pointer);
    if (elf_kind_val != ELF_K_ELF)
        return;
    // get section header index
    if (elf_getshdrstrndx(elf_pointer, &section_heaher_index) != 0)
        return;

    // Get entrypoint header
    GElf_Ehdr _ehdr;
    GElf_Ehdr *ehdr = gelf_getehdr(elf_pointer, &_ehdr);
    // set entrypoint
    *start_addr = ehdr->e_entry;

    int section_index = 0;

    // Iterate through each section
    while ((elf_scn = elf_getscn(elf_pointer, section_index)) != NULL)
    {
        elf_shdr = elf32_getshdr(elf_scn);
        // check if the section needs to be allocated on memory
        if ((elf_shdr->sh_flags & SHF_ALLOC) && (elf_shdr->sh_size > 0))
        {
            elf_data = elf_getdata(elf_scn, NULL);
            string region_name = elf_strptr(elf_pointer, section_heaher_index, elf_shdr->sh_name);
            if (base_addr > elf_shdr->sh_addr)
                base_addr = elf_shdr->sh_addr;
            regmgr.add_region({elf_shdr->sh_addr, (elf_shdr->sh_addr + elf_shdr->sh_size), elf_shdr->sh_size, 0, region_name});
        }
        section_index++;
    }
    section_index = 0;
    regmgr.init_regions();
    regmgr.print_region_info();
    *start_addr = regmgr.get_mem_address(*start_addr);
    uint32_t msize = regmgr.get_memory_size();
    *total_memory_size = msize;
    *mem = new uint8_t[msize];
    elf_pointer = elf_begin(elf_fd, ELF_C_READ, NULL);
    if (elf_pointer == NULL)
        cout << "elf_pointer: " << elf_pointer << " " << endl;
    while ((elf_scn = elf_getscn(elf_pointer, section_index)) != NULL)
    {
        string region_name = elf_strptr(elf_pointer, section_heaher_index, elf_shdr->sh_name);
        cout << region_name << hex << " start addr: " << elf_shdr->sh_addr << " end addr: " << (elf_shdr->sh_addr + elf_shdr->sh_size) << endl;
        elf_shdr = elf32_getshdr(elf_scn);

        if ((elf_shdr->sh_flags & SHF_ALLOC) && (elf_shdr->sh_size > 0))
        {
            // check if current section is text section
            if (elf_shdr->sh_type == SHT_PROGBITS)
            {
                elf_data = elf_getdata(elf_scn, NULL);
                uint64_t currr_addr = elf_shdr->sh_addr;
                for (int i = 0; i < elf_shdr->sh_size; i++)
                {
                    (*mem)[regmgr.get_mem_address(elf_shdr->sh_addr + i)] = ((uint8_t *)elf_data->d_buf)[i];
                }
            }
        }
        section_index++;
    }
    elf_end(elf_pointer);
    close(elf_fd);
}

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