#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <elfio/elf_types.hpp>
#include <fcntl.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/user.h>
#include <tuple>
#include <signal.h>

#include "elf_loader.hpp"
#include "log.hpp"
#include "utils.hpp"

namespace elfLoader {
void ElfLoader::loadElf(const std::string &path, size_t stackSize) {

    LOADER_LOG("Attempting to read {} elf file data...", path);
    readElfFile(path);

    LOADER_LOG("Acquiring {} descriptor...", path);
    int elfFd = openElfFile(path);
   
    parseSegments();
    mapSegments(elfFd);

    // TODO should deal with vdso?

    // TODO unmap signal handlers, fds, etc...
    cleanupLoaderResources();

    // TODO PR_SET_MM_EXE_FILE, PR_SET_MM_MAP

    runTrampoline();
}

void ElfLoader::parseSegments() {
    LOADER_LOG("Parsing segments:");
    
    m_loadStartAddress = UINT64_MAX;
    m_loadEndAddress   = 0;

    m_stackProtection = PROT_READ | PROT_WRITE;


    for (const auto &segment : m_elfReader.segments) {
        LOADER_LOG("\tFound segment with type 0x{:x}", segment->get_type());

        switch (segment->get_type()) {
            case ELFIO::PT_LOAD: {
                const auto segmentStart = segment->get_virtual_address();
                const auto segmentEnd   = segmentStart + segment->get_memory_size();

                LOADER_LOG("\t|\tSegment type determined as a PT_LOAD\n"
                           "\t|\tStart: 0x{:016x}\n"
                           "\t|\tEnd:   0x{:016x}", segmentStart, segmentEnd);

                m_loadStartAddress = std::min(m_loadStartAddress, segmentStart);
                m_loadEndAddress   = std::max(m_loadEndAddress,   segmentEnd);
                break;
            }
            
            case ELFIO::PT_GNU_STACK:
                // TODO check for a single stack segment
                LOADER_LOG("\t|\tSegment type determined as a PT_GNU_STACK");

                if (segment->get_flags() & ELFIO::PF_X) {
                    LOADER_LOG("\t|\tAllowing execution in the stack mapping");
                    m_stackProtection |= PROT_EXEC;
                }
                break;

            // TODO do smth with GNU_EH_FRAME?
            case ELFIO::PT_GNU_EH_FRAME:
                LOADER_LOG("\t|\tSegment type determined as a PT_GNU_EH_FRAME\n"
                           "\t|\tJust chilling :D");
                break;
            
            default:
                throw std::runtime_error(fmt::format("Segment with unknown type 0x{:x} has been found", segment->get_type()));
        };

        LOADER_LOG("\t+-------------------------------\n");
    }
}

void ElfLoader::mapSegments(int elfFd) {
    LOADER_LOG("Mapping segments:");

    for (const auto &segment : m_elfReader.segments) {
        // TODO also allocate stack?
        if (segment->get_type() != ELFIO::PT_LOAD) {
            continue;
        }

        LOADER_LOG("\tLoading another segment");

        mapPtLoadSegment(segment, elfFd);

        LOADER_LOG("\t+-------------------------------\n");
   }
}

void ElfLoader::mapPtLoadSegment(const std::unique_ptr<ELFIO::segment> &segment, int elfFd) {
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
        LOADER_LOG("\t|\tAllocating memory...");

        // TODO is allocating whole page safe (are segments aligned in elf itself)?
        std::ignore = mmap(reinterpret_cast<void *>(pageAddress), fileSize, mmapProtection,
                           MAP_FIXED | (mmapProtection & PROT_WRITE ? MAP_PRIVATE : MAP_SHARED), 
                           elfFd, filePageAddress);
    }

    // mappingSize > fileSize => extra .bss pages have to be mapped
    if (mappingSize > fileSize) {
        LOADER_LOG("\t|\tAllocating extra .bss pages...");

        // Extra peges are already zeroed
        std::ignore = mmap(reinterpret_cast<void *>(pageAddress + fileSize), mappingSize - fileSize, 
                           mmapProtection, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
    }

    // memsz > filesz => .bss exists and has to be zeroed
    if (segment->get_memory_size() > segment->get_file_size()) {
        LOADER_LOG("\t|\tZeroing .bss section...");

        const auto fileMappingEnd = virtualAddress + segment->get_file_size();
        const auto zeroChunkEnd   = alignUp(fileMappingEnd, PAGE_SIZE);

        if (zeroChunkEnd > fileMappingEnd) {
            std::memset(reinterpret_cast<void *>(fileMappingEnd), 0, zeroChunkEnd - fileMappingEnd);
        }
    }
}

void ElfLoader::cleanupLoaderResources() {
    stack_t signalStack = { .ss_flags = SS_DISABLE };

    if (sigaltstack(&signalStack, nullptr) == -1) {
        throw std::runtime_error("Can't disable alternative signal stack");
    }

    struct sigaction oldSignal = {};
    constexpr size_t signalsCount = sizeof(oldSignal.sa_mask) * CHAR_BIT;

    for(size_t signalNumber = 0; signalNumber <= signalsCount; signalNumber++) {
        
    }
}

void ElfLoader::runTrampoline() {
    LOADER_LOG("============================================================\n"
               "==================[ Switching executable ]==================\n"
               "============================================================\n");
    
    const uint64_t newIp = m_elfReader.get_entry();
    
#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64)
    __asm__ __volatile__ (
        "jmp *%0"
        :: "r"(newIp), "d"(0)
    );
#endif
    
    __builtin_unreachable();

}

int ElfLoader::openElfFile(const std::string &path) {
    int elfFd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);

    if (elfFd < 0) {
        throw std::runtime_error(fmt::format("Can not open file {}", path));
    }

    LOADER_LOG("Got descriptor {} for the file {}", elfFd, path);

    return elfFd;
}

void ElfLoader::readElfFile(const std::string &path) {
    if (!m_elfReader.load(path)) {
        throw std::runtime_error(fmt::format("File {} is not found or it is not an ELF file", path));
    }

    if (m_elfReader.get_type() != ELFIO::ET_EXEC) {
        throw std::runtime_error("Only ET_EXEC elf files are currently supported");
    }

    LOADER_LOG("Elf file {} read successfully!", path);
    // TODO additional format checks
}
} // namespace elfLoader
