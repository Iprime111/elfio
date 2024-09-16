#ifndef ELF_LOADER_HPP_
#define ELF_LOADER_HPP_

#include <elfio/elfio.hpp>
#include <sys/mman.h>

#include "stack_utils.hpp"

namespace elfLoader {
constexpr size_t kDefaultStackSize = 0xcc000;

class ElfLoader {
  public:
    ElfLoader(const char *argv[], char *envp[]) : m_stackBottom(argv, envp) {}

    void loadElf(const std::string &path, size_t stackSize = kDefaultStackSize);

  private:
    void readElfFile(const std::string &path);
    int  openElfFile(const std::string &path);

    void parseSegments();

    void runTrampoline         ();
    void cleanupLoaderResources();

    void mapSegments(int elfFd);
    void mapPtLoadSegment(const std::unique_ptr<ELFIO::segment> &segment, int elfFd);

    ELFIO::elfio m_elfReader = {};

    int               m_stackProtection  = PROT_NONE;
    ELFIO::Elf64_Addr m_loadStartAddress = 0;
    ELFIO::Elf64_Addr m_loadEndAddress   = 0;

    StackData m_stackBottom;
};
} // namespace elfLoader

#endif
