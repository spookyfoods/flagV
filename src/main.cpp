#include "loader.h"
#include "system.h"

int main(int argc, char* argv[]) {
    // Default to 'fact.bin' if no file is provided as an argument
    std::string binary_path = (argc > 1) ? argv[1] : "fact.bin";

    try {
        CPU cpu(1 << 20);
        cpu.load(loader::read_words(argv[1]), 0x80000000);
        if (cpu.run(100'000) == ExecResult::Halt) {
            const uint32_t a0 = cpu.reg(10);
            std::fprintf(stderr, "EXIT %u\n", a0);
            return static_cast<int>(a0 & 0xFF); // 0 = pass
        }
        return 200; // fault or step limit
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 201; // e.g. unknown opcode
    }

    return 0;
}
