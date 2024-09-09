#include "elf_loader.hpp"
#include <elfio/elfio.hpp>
#include <fmt/format.h>

using namespace ELFIO;

extern char **environ;

int main(int argc, const char** argv)
{
    if (argc != 2) {
        fmt::println("Usage: elf-loader <file_name>");
        return 1;
    }

    elfLoader::ElfLoader loader{argv, environ};

    loader.loadElf(argv[1]);

    return 0;
}
