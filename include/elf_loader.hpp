#ifndef ELF_LOADER_HPP_
#define ELF_LOADER_HPP_

#include <elfio/elfio.hpp>

#include "stack_utils.hpp"

namespace elfLoader {
constexpr size_t kDefaultStackSize = 0xcc000;

class ElfLoader {
  public:
    void loadElf(std::string &path, const StackData &stackStart, size_t stackSize = kDefaultStackSize);

    void executeElf();

    bool isLoaded() const { return m_isLoaded; }

  private:
    void readElfFile(std::string &path);

    bool m_isLoaded = false;

    ELFIO::elfio m_elfReader = {};
    uint8_t *m_elfContent    = nullptr;
};
} // namespace elfLoader

#endif
