#include "system.h"
#include <cstdlib>
#include <iostream>

Memory::Memory(size_t size, Counters& _ctrs) : data(size, 0), ctrs(_ctrs) {}

size_t Memory::translate(uint32_t address, uint32_t pc, uint32_t width) const {
    if (address < MEM_BASE) {
        fail_loudly(address, pc, width, "Address Below MEM_BASE");
    }
    size_t offset = address - MEM_BASE;
    if (offset + width > data.size()) {
        fail_loudly(address, pc, width, "Address out of upper bounds");
    }
    return offset;
}

void Memory::fail_loudly(uint32_t address, uint32_t pc, uint32_t width,
                         const char* reason) const {
    std::cerr << "[MEMORY FAULT] " << reason << " | Addr: 0x" << std::hex
              << address << " | PC: 0x" << pc << " | Width: " << std::dec
              << width << " bytes\n";
    std::abort();
}

uint32_t Memory::fetch32(uint32_t address) {
    size_t offset = translate(address, address, 4);
    ctrs.inc_icount();
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

uint8_t Memory::read8(uint32_t address, uint32_t pc) {
    size_t offset = translate(address, pc, 1);
    ctrs.inc_loads();
    return data[offset];
}

uint32_t Memory::read32(uint32_t address, uint32_t pc) {
    size_t offset = translate(address, pc, 4);
    ctrs.inc_loads();
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

void Memory::write8(uint32_t address, uint32_t pc, uint8_t val) {
    size_t offset = translate(address, pc, 1);
    ctrs.inc_stores();
    data[offset] = val;
}

void Memory::write32(uint32_t address, uint32_t pc, uint32_t val) {
    size_t offset = translate(address, pc, 4);
    ctrs.inc_stores();
    for (int i = 0; i < 4; i++) {
        data[offset + i] = static_cast<uint8_t>(0x000000FF & val);
        val >>= 8;
    }
}

CPU::CPU(int _memSize)
    : ctrs(), regs(), mem(_memSize, ctrs), pc(0), halted(false) {}
void CPU::step() {
    auto word = mem.fetch32(pc);
    if (word == 0) {
        halted = true;
        return;
    }
    Instruction ins = decode(word);

    int32_t pcInc = 4;
    ExecResult r = execute_context(ins, pcInc);
    pc = pc + pcInc;
    ctrs.inc_icount();
    if (r == ExecResult::Fault)
        halted = true;
}

void CPU::run() {
    while (!halted) {
        // ==Breakpoint==
        step();
    }
}
void CPU::load(std::vector<uint32_t> insVec, uint32_t startAddress) {
    pc = startAddress;
    for (int i = 0; i < insVec.size(); i++) {
        mem.write32(startAddress + 4 * i, pc, insVec[i]);
    }
}
// Temporary
const Memory& CPU::getMemRef() { return mem; }

void dump_state(const CPU& cpu) {

    // x0  zero    x8  s0/fp   x16 a6     x24 s8
    // x1  ra      x9  s1      x17 a7     x25 s9
    // x2  sp      x10 a0      x18 s2     x26 s10
    // x3  gp      x11 a1      x19 s3     x27 s11
    // x4  tp      x12 a2      x20 s4     x28 t3
    // x5  t0      x13 a3      x21 s5     x29 t4
    // x6  t1      x14 a4      x22 s6     x30 t5
    // x7  t2      x15 a5      x23 s7     x31 t6

    static constexpr std::array<const char*, 32> ABI_NAMES = {
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

    std::println("{:<7}= 0x{:08x}  |  icount={:<5} loads={:<5} stores={:<5}",
                 "pc", cpu.pc, cpu.ctrs.get_loads(), cpu.ctrs.get_loads(),
                 cpu.ctrs.get_stores());
    std::println();

    for (int i = 1; i < 32; i++) {
        std::println("x{:<2}/{:<4} 0x{:08x}", i, ABI_NAMES[i],
                     cpu.regs.read(i));
    }
}
