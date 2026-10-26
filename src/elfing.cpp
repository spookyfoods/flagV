#include <fstream>
#include <iostream>
#include <print>
#include <string>
constexpr std::array<unsigned char, 4> elf_magic{0x7F, 'E', 'L', 'F'};

int main() {

    std::ifstream elf32exe("/Users/spookymac/src/riscv-tests/isa/rv32ui-p-add");
    if (elf32exe.is_open()) {
        std::print("Opened File Succesfully!\n");
    }

    std::istreambuf_iterator<char> it(elf32exe);

    // Parse `unsigned char e_ident[EI_NIDENT];`
    for (auto i = 0uz; i < 4; ++i, ++it) {
        const auto ch = static_cast<unsigned char>(*it);
        if (ch != elf_magic[i]) {
            std::println("Error!");
            return -1;
        }
    }
    // Skip till end of e_ident
    for (auto i = 0uz; i < 12; ++i, ++it)
        ;

    // Parse uint16_t e_type;
    const auto ch = static_cast<uint16_t>(*it++);
    std::println("{}", ch);

    return 0;
}
