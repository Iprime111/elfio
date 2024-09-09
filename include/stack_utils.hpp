#ifndef STACK_UTILS_HPP_
#define STACK_UTILS_HPP_

#include <cstddef>
#include <elfio/elfio.hpp>

namespace elfLoader {
class StackData {
  public:
    // TODO use Elf32_auxv here?
    StackData(const char *argv[], char *envp[]);

    size_t getArgCount() const { return m_argCount; }
    size_t getEnvCount() const { return m_envCount; }
    size_t getAuxCount() const { return m_auxCount; }

    size_t getArgsSize()  const { return m_argsSize;  }
    size_t getStackSize() const { return m_stackSize; }

    const char             **getArgv() const { return m_argv; }
          char             **getEnvp() const { return m_envp; }
    const ELFIO::Elf64_auxv *getAuxv() const { return m_auxv; }

  private:
    size_t m_argCount;
    size_t m_envCount;
    size_t m_auxCount;

    size_t m_argsSize;
    size_t m_stackSize;

    const char             **m_argv;
          char             **m_envp;
    const ELFIO::Elf64_auxv *m_auxv;

};

} // namespace elfLoader
#endif
