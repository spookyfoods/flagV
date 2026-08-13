#include "decode.h"
#include "opcodes.h"
#include "system.h"
#include <iostream>

void test_bitwise(RegisterFile& rf) {
    // lui x1, 0xF0F0F  --> x1 = 0xF0F0F000
    machine_code(rf, decode(0xf0f0f0b7));
    // lui x2, 0xFF000  --> x2 = 0xFF000000
    machine_code(rf, decode(0xff000137));

    // and x3, x1, x2   --> x3 = x1 & x2
    machine_code(rf, decode(0x0020f1b3));
    std::cout << "AND Expected: f0000000 | Actual: " << std::hex << rf.read(3)
              << '\n';

    // or  x4, x1, x2   --> x4 = x1 | x2
    machine_code(rf, decode(0x0020e233));
    std::cout << "OR  Expected: ff0f0000 | Actual: " << std::hex << rf.read(4)
              << '\n';

    // xor x5, x1, x2   --> x5 = x1 ^ x2
    machine_code(rf, decode(0x0020c2b3));
    std::cout << "XOR Expected: 0f0f0000 | Actual: " << std::hex << rf.read(5)
              << '\n';
}

int main() { std::cout << std::hex << *get_opcode(decode(0xf0f0f0b7)); }
