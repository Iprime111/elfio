#include <cstdint>
#include <cstring>
#include <elfio/elf_types.hpp>
#include <fcntl.h>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/os.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/user.h>

#include "elf_loader.hpp"
#include "log.hpp"
#include "utils.hpp"

namespace elfLoader {
void ElfLoader::loadElf(const std::string &path, size_t stackSize) {

    // TODO deal with stack?
    LOADER_LOG("Reading elf file...");
    readElfFile(path);

    LOADER_LOG("Acquiring elf file descriptor...");
    openElfFile(path);
    
    LOADER_LOG("Elf fd: {}", m_elfFd);

    ELFIO::Elf64_Addr startAddress = UINT64_MAX;
    ELFIO::Elf64_Addr endAddress   = 0;

    int stackProtection = PROT_READ | PROT_WRITE;

    LOADER_LOG("Parsing segments:");

    for (const auto &segment : m_elfReader.segments) {
        LOADER_LOG("\tfound segment with type {}", segment->get_type());

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

            // TODO do smth with GNU_EH_FRAME?
            case ELFIO::PT_GNU_EH_FRAME:
                break;
            
            default:
                throw std::runtime_error(fmt::format("Non-static segment has been found: 0x{:x}", segment->get_type()));
        };
    }

    // TODO should deal with vdso?

    /* TODO Do I need this code (commented for now)?
    const size_t stackDataSize   = alignUp(stackSize,                    PAGE_SIZE);
    const size_t stackBottomSize = alignUp(m_stackBottom.getStackSize(), PAGE_SIZE);

    const size_t stackAllocationSize = stackDataSize + stackBottomSize;

    void *stackAllocation = 
        mmap(0, stackAllocationSize, stackProtection, MAP_GROWSDOWN | MAP_STACK | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    if (stackAllocation == MAP_FAILED) {
        throw std::runtime_error("Stack allocation error");
    }
    
    // allocate and build stack here
    buildStack(static_cast<uint8_t *>(stackAllocation), stackDataSize, stackBottomSize); */
    
    LOADER_LOG("Loading segments:");

    for (const auto &segment : m_elfReader.segments) {
        if (segment->get_type() != ELFIO::PT_LOAD) {
            continue;
        }

        LOADER_LOG("\tLoading another segment");

        // TODO maybe rewrite to allocate pages more efficiently
        const auto virtualAddress  = segment->get_virtual_address();
        const auto pageOffset      = findMisalignment(virtualAddress, PAGE_SIZE);
        const auto pageAddress     = virtualAddress - pageOffset;
        const auto mappingSize     = alignUp(virtualAddress + segment->get_memory_size(), PAGE_SIZE) - pageAddress;
                  
        const auto fileAddress     = segment->get_offset();
        const auto fileOffset      = findMisalignment(fileAddress, PAGE_SIZE);
        const auto filePageAddress = fileAddress - fileOffset;
        const auto fileSize        = alignUp(fileAddress + segment->get_file_size(), PAGE_SIZE) - filePageAddress;

        int mmapProtection = getMmapProtection(segment->get_flags());

        // TODO make maps after other operations?

        // TODO what if adress range is already mapped?
        // TODO do this after unmapping loader?
        if (fileSize > 0) {
            LOADER_LOG("\t|\tAllocating memory");

            // TODO is allocating whole page safe (are segments aligned in elf itself)?
            void *_ = mmap(reinterpret_cast<void *>(pageAddress), fileSize, mmapProtection,
                           MAP_FIXED | (mmapProtection & PROT_WRITE ? MAP_PRIVATE : MAP_SHARED), m_elfFd, filePageAddress);
        }

        // mappingSize > fileSize => extra .bss pages have to be mapped
        if (mappingSize > fileSize) {
            LOADER_LOG("\t|\tAllocating extra .bss pages");

            // Extra peges are already zeroed
            void *_ = mmap(reinterpret_cast<void *>(pageAddress + fileSize), mappingSize - fileSize, mmapProtection,
                           MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
        }

        // memsz > filesz => .bss exists and has to be zeroed
        if (segment->get_memory_size() > segment->get_file_size()) {
            LOADER_LOG("\t|\tZeroing .bss");

            const auto fileMappingEnd = virtualAddress + segment->get_file_size();
            const auto zeroChunkEnd   = alignUp(fileMappingEnd, PAGE_SIZE);

            if (zeroChunkEnd > fileMappingEnd) {
                std::memset(reinterpret_cast<void *>(fileMappingEnd), 0, zeroChunkEnd - fileMappingEnd);
            }
        }

        LOADER_LOG("\t+-------------------------------\n");
   }


    // TODO unmap signal handlers, fds, etc...

        // TODO PR_SET_MM_EXE_FILE, PR_SET_MM_MAP

        LOADER_LOG("Switching executable\n"
                   "------------------------------------------------------------");

        const uint64_t newIp = m_elfReader.get_entry();

        __asm__ __volatile__ (
            "jmp *%0"
            :: "r"(newIp), "d"(0)
        );

        __builtin_unreachable();

}

uint8_t *ElfLoader::openElfFile(const std::string &path) {
    const auto fileSize = std::filesystem::file_size(path);

    m_elfFd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);

    if (m_elfFd < 0) {
        throw std::runtime_error(fmt::format("Can not open file {}", path));
    }

    //uint8_t *elfMapping = static_cast<uint8_t *>(::mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, m_elfFd, 0));
    //
    //if (elfMapping == MAP_FAILED) {
    //    throw std::runtime_error("Can not allocate memory for a file");
    //}
    //
    //return elfMapping;
    
    return nullptr;
}

void ElfLoader::buildStack(uint8_t *stackAllocation, size_t stackDataSize, size_t stackBottomSize) {
    uint8_t *bottomStart = stackAllocation + stackDataSize;

    // Set auxv end
    *(reinterpret_cast<uint64_t *>(bottomStart) - 1) = ELFIO::AT_NULL;
    *(reinterpret_cast<uint64_t *>(bottomStart) - 2) = ELFIO::AT_NULL;

    uint8_t *dataPtr = bottomStart;

    // Copy auxv
    for (const ELFIO::Elf64_auxv *auxPtr = m_stackBottom.getAuxv(); auxPtr->a_type != ELFIO::AT_NULL; auxPtr++) {
        ELFIO::Elf64_auxv auxValue = *auxPtr;

        switch (auxPtr->a_type) {
            case ELFIO::AT_RANDOM:
                auxValue.a_un.a_val = reinterpret_cast<uint64_t>(dataPtr);

                dataPtr += 16;
                break;
            // TODO stopping to write it 'cause argv, envp and auxv may be unused
        }
    }
}

void ElfLoader::readElfFile(const std::string &path) {
    if (!m_elfReader.load(path)) {
        throw std::runtime_error(fmt::format("File {} is not found or it is not an ELF file", path));
    }

    // TODO additional format checks
}

} // namespace elfLoader
