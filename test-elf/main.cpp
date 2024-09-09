#include <cstddef>
#include <syscall.h>

static void exit(int exitCode);
static long write(int fd, const void *buffer, size_t size);

extern "C" void _start() {
    const char testString[] = "abc\n";

    write(1, testString, sizeof(testString) - 1);

    int number = 12345;

    while (number > 0) {
        char digit = (number % 10) + '0';
        number /= 10;

        write(1, &digit, 1);
    }

    exit(0);
}

static long write(int fd, const void *buffer, size_t size) {
    long result = -1;

    __asm__ __volatile__ (
        "syscall"
        : "=a"(result)
        : "0"(__NR_write), "D"(fd), "S"(buffer), "d"(size)
        : "cc", "rcx", "r11", "memory"
    );

    return result;
}

static void exit(int exitCode) {
    __asm__ __volatile__ (
        "syscall"
        :
        : "a"(__NR_exit)
        : "cc", "rcx", "r11", "memory"
    );

    __builtin_unreachable();
}
