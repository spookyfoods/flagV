#include "loader.h"
#include "system.h"

#include <cstdio>
#include <exception>
#include <string>

namespace {
    constexpr int kMemSize = 1 << 20;              // 1 MB
    constexpr uint32_t kStartAddress = 0x00000000; // must match -Ttext
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <program.bin>\n", argv[0]);
        return 2;
    }

    try {
        CPU cpu(kMemSize);
        cpu.load(loader::read_words(argv[1]), kStartAddress);
        cpu.run();
        dump_state(cpu);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
