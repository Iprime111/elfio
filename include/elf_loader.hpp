#ifndef ELF_LOADER_HPP_
#define ELF_LOADER_HPP_

#include <cstdint>
#include <elfio/elfio.hpp>

#include "stack_utils.hpp"

namespace elfLoader {
constexpr size_t kDefaultStackSize = 0xcc000;

class ElfLoader {
  public:
    ElfLoader(const char *argv[], char *envp[]) : m_stackBottom(argv, envp) {}

    void loadElf(const std::string &path, size_t stackSize = kDefaultStackSize);

    void executeElf();

    bool isLoaded() const { return m_isLoaded; }

  private:
    void readElfFile(const std::string &path);

    uint8_t *openElfFile(const std::string &path);

    void buildStack(uint8_t *stackAllocation, size_t stackDataSize, size_t stackBottomSize);

    bool m_isLoaded = false;
    int  m_elfFd    = -1;

    ELFIO::elfio m_elfReader = {};
    uint8_t *m_elfContent    = nullptr;

    StackData m_stackBottom;
};
} // namespace elfLoader

#endif
