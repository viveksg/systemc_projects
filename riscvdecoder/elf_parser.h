#ifndef __ELF_PARSER__
#define __ELF_PARSER__
#include <libelf.h>
#include <fcntl.h>
#include <gelf.h>
#include <bfd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <systemc.h>
#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
using namespace std;
typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t region_size;
    uint32_t region_base;
    string region_name;
}region;

class RegionManager{
protected:   
    vector<region>regions;
    uint32_t start_addr;
public:
    uint32_t total_memory_size;
    RegionManager();
    void add_region(region mem_region);
    void init_regions();
    uint32_t get_mem_address(uint32_t addr);
    void print_region_info();
    uint32_t get_memory_size();
    uint32_t get_start_address();
};

class ELFParser
{
public:
    ELFParser(string file_location, uint32_t *start_addr, uint8_t** mem, uint32_t*total_memory_size);
};
#endif
