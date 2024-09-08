#include <algorithm>
#include <cstdint>
#include <elfio/elf_types.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/user.h>

#include "elf_loader.hpp"
#include "utils.hpp"

// Undefine constants used in elfio
#undef PT_LOAD
#undef PT_GNU_STACK
#undef PF_X

namespace elfLoader {
void ElfLoader::loadElf(std::string &path, const StackData &stackStart, size_t stackSize) {
    const auto fileSize = std::filesystem::file_size(path);

    // TODO DEAL WITH STACK

    readElfFile(path);

    ELFIO::Elf64_Addr startAddress = UINT64_MAX;
    ELFIO::Elf64_Addr endAddress   = 0;

    int stackProtection = PROT_READ | PROT_WRITE;

    for (const auto &segment : m_elfReader.segments) {
        switch (segment->get_type()) {
        case ELFIO::PT_LOAD:
            startAddress = std::min(startAddress, segment->get_virtual_address());
            endAddress   = std::max(endAddress,   segment->get_virtual_address() + segment->get_memory_size());
            break;

        case ELFIO::PT_GNU_STACK:
            if (segment->get_flags() & ELFIO::PF_X) {
                stackProtection |= PROT_EXEC;
            }
            break;

        default:
            throw std::runtime_error("Non-static section has been found");
        };
    }

    // TODO should deal with vdso?
    
    const size_t stackAllocationSize = alignUp(stackSize, PAGE_SIZE) + alignUp(stackStart.getStackSize(), PAGE_SIZE);

    void *stackAllocation = 
        mmap(0, stackAllocationSize, stackProtection, MAP_GROWSDOWN | MAP_STACK | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    if (stackAllocation == MAP_FAILED) {
        throw std::runtime_error("Stack allocation error");
    }

    // TODO allocate and build stack here
    
    
}

void ElfLoader::readElfFile(std::string &path) {
    if (!m_elfReader.load(path)) {
        throw std::runtime_error(fmt::format("File {} is not found or it is not an ELF file", path));
    }

    // TODO additional format checks
}

} // namespace elfLoader
