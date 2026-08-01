#include "decode.h"
#include <iostream>

int main() {
    if (auto rd = get_rd(decode(0x00b51463)); rd.has_value()) {
        // Dereference with * or .value()
        std::cout << "rd register: x" << static_cast<int>(*rd) << '\n';
    } else {
        std::cout << "Instruction does not write to a destination register.\n";
    }
}
