#include "system.h"
#include <iostream>
#ifndef TRACE_H
#define TRACE_H
template <class Instruction>
inline void trace(const Instruction& ins, const CPU* this_cpu) {
    std::cout << "EXECUTE_CONTEXT_VERBOSE is TRUE\n";
    std::cerr << ins << '\n';
    dump_state(*this_cpu);
}

#endif // TRACE_H
