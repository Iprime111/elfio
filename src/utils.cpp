#include <cstdint>
#include <sys/mman.h>

#include "utils.hpp"

int getMmapProtection(ELFIO::Elf_Word flags) {
    int protection = 0;

    if (flags & ELFIO::PF_X) protection |= PROT_EXEC; 
    if (flags & ELFIO::PF_W) protection |= PROT_WRITE; 
    if (flags & ELFIO::PF_R) protection |= PROT_READ; 

    return protection;
}

uint64_t alignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

uint64_t alignDown(uint64_t value, uint64_t alignment) {
    return value / alignment * alignment;
}

uint64_t findMisalignment(uint64_t value, uint64_t alignment) {
    return value % alignment;
}

bool isAligned(uint64_t value, uint64_t alignment) {
    return value % alignment == 0;
}
