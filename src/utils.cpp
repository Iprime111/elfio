#include <cstdint>

#include "utils.hpp"

uint64_t alignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

uint64_t alignDown(uint64_t value, uint64_t alignment) {
    return value / alignment * alignment;
}

bool isAligned(uint64_t value, uint64_t alignment) {
    return value % alignment == 0;
}
