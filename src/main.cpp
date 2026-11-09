#include "loader.h"
#include "system.h"

#include <cstdio>
#include <exception>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s <program.bin> --tohost <address>\n",
                     argv[0]);
        return 2;
    }

    try {
        CPU cpu(1 << 24);
        cpu.set_tohost(static_cast<uint32_t>(std::stoul(argv[3], nullptr, 0)));
        cpu.load(loader::read_words(argv[1]), 0x80000000);

        if (cpu.run(10'000'000) != ExecResult::Halt) {
            return 200; // fault or step limit
        }

        const uint32_t value = cpu.tohost_value();
        if (value == 1) {
            return 0; // pass
        }
        std::fprintf(stderr, "EXIT %u\n", value);
        return value == 0 ? 202 : static_cast<int>(value & 0xFF);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 201;
    }
}
