#include <elfio/elfio.hpp>
#include <fmt/format.h>

using namespace ELFIO;

int main(int argc, char** argv)
{
    if (argc != 2) {
        fmt::println("Usage: elf-loader <file_name>");
        return 1;
    }



    fmt::println("Elf type: {}\nVirtual entry address: {}", reader.get_type(), reader.get_entry());

    

    return 0;
}
