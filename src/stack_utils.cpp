#include <cstring>
#include <elfio/elfio.hpp>

#include "stack_utils.hpp"

namespace {
size_t getArgsSize(const char *const args[], size_t &count) {
    size_t argsSize = 0;
    count = 0;

    for (const char *const *arg = args; *arg; arg++) {
        argsSize += strlen(*arg) + 1;
        count++;
    }

    return argsSize;
}

size_t getAuxvSize(const ELFIO::Elf64_auxv* auxv, size_t &count) {
    size_t argsSize = 0;
    count = 0;

    for (const ELFIO::Elf64_auxv* auxp = auxv; auxp->a_type != ELFIO::AT_NULL; auxp++) {
        switch (auxp->a_type) {
            case ELFIO::AT_RANDOM:
            argsSize += 16; // TODO get rid of magic number
            break;
            case ELFIO::AT_PLATFORM:
            argsSize += strlen(reinterpret_cast<char *>(auxp->a_un.a_val)) + 1;
            break;
        default:
            break;
        // TODO other cases?
        // TODO AT_EXECFN
        }
        count++;
    }

    return argsSize;
}

const ELFIO::Elf64_auxv *getAuxv(char **envp) {
    while(*envp++);

    return reinterpret_cast<const ELFIO::Elf64_auxv *>(envp);
}
} // namespace

namespace elfLoader {
StackData::StackData(const char *argv[], char *envp[]) : 
    m_argv(argv), m_envp(envp), m_auxv(::getAuxv(envp)) {

    m_argsSize = 0;

    m_argsSize += ::getArgsSize(m_argv, m_argCount);
    m_argsSize += ::getArgsSize(m_envp, m_envCount);
    m_argsSize += ::getAuxvSize(m_auxv, m_auxCount);

    // round value
    m_argsSize = (m_argsSize + 7) / 8;

    // Why this?
    size_t stackWords = 2 * (1 + m_auxCount) + (1 + m_envCount) + (1 + m_auxCount) + 1;
    
    // If we have an odd number of words left to push and the stack is
    // currently 16 byte aligned, misalign the stack by 8 bytes.
    // And vice versa.
    if (!(stackWords & 1) != !(m_argsSize & 8)) { // What the fuck?...
        stackWords++;
    }

    m_stackSize = m_argsSize + stackWords * 8;
}
} // namespace elfLoader
