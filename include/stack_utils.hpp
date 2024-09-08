#ifndef STACK_UTILS_HPP_
#define STACK_UTILS_HPP_

#include <cstddef>
#include <elf.h>

namespace elfLoader {
class StackData {
  public:
    // TODO use Elf32_auxv_t here?
    StackData(const char *const argv[], const char *const envp[], const Elf64_auxv_t* auxv);

    size_t getArgCount() const { return m_argCount; }
    size_t getEnvCount() const { return m_envCount; }
    size_t getAuxCount() const { return m_auxCount; }

    size_t getArgsSize()  const { return m_argsSize;  }
    size_t getStackSize() const { return m_stackSize; }

  private:
    size_t m_argCount;
    size_t m_envCount;
    size_t m_auxCount;

    size_t m_argsSize;
    size_t m_stackSize;

};

} // namespace elfLoader
#endif
