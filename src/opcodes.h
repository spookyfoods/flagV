#ifndef OPCODES_H
#define OPCODES_H
#include "decode.h"
#include "system.h"

auto machine_code(RegisterFile& rf, const Instruction& ins)
    -> std::optional<bool>;
#endif // OPCODES_H
