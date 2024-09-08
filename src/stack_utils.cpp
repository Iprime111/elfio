#include <cstring>

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

size_t getAuxvSize(const Elf64_auxv_t* auxv, size_t &count) {
    size_t argsSize = 0;
    count = 0;

    for (const Elf64_auxv_t* auxp = auxv; auxp->a_type != AT_NULL; auxp++) {
        switch (auxp->a_type) {
        case AT_RANDOM:
            argsSize += 16; // TODO get rid of magic number
            break;
        case AT_PLATFORM:
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
} // namespace

namespace elfLoader {
StackData::StackData(const char *const argv[], const char *const envp[], const Elf64_auxv_t* auxv) {
    m_argsSize = 0;

    m_argsSize += ::getArgsSize(argv, m_argCount);
    m_argsSize += ::getArgsSize(envp, m_envCount);
    m_argsSize += ::getAuxvSize(auxv, m_auxCount);

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
