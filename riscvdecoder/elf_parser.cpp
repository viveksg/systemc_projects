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
