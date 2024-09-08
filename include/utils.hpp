#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstdint>

uint64_t alignUp  (uint64_t value, uint64_t alignment);
uint64_t alignDown(uint64_t value, uint64_t alignment);

bool isAligned(uint64_t value, uint64_t alignment);
#endif
