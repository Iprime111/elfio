#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstdint>
#include <elfio/elfio.hpp>

int getMmapProtection(ELFIO::Elf_Word flags);

uint64_t alignUp         (uint64_t value, uint64_t alignment);
uint64_t alignDown       (uint64_t value, uint64_t alignment);
uint64_t findMisalignment(uint64_t value, uint64_t alignment);

bool isAligned(uint64_t value, uint64_t alignment);
#endif
