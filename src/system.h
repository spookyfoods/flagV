#ifndef SYSTEM_H
#define SYSTEM_H

#include "decode.h"
#include <array>
#include <cstdint>
#include <vector>

enum class ExecResult {
    Fault,
    Continue,
    Halt,
};
class Counters {
  private:
    uint64_t icount{0};
    uint64_t loads{0};
    uint64_t stores{0};

  public:
    // Counter getters
    uint64_t get_icount() const { return icount; }
    uint64_t get_loads() const { return loads; }
    uint64_t get_stores() const { return stores; }

    // Counter mutators used by Memory primitives
    void inc_icount() { icount++; }
    void inc_loads() { loads++; }
    void inc_stores() { stores++; }
};
class RegisterFile {
  private:
    std::array<uint32_t, 32> regs{};

  public:
    RegisterFile() = default;
    constexpr void write(uint8_t index, uint32_t value) {
        if (index == 0) {
            return;
        }
        regs[index] = value;
    }

    constexpr uint32_t read(uint8_t index) const { return regs[index]; }
    constexpr uint32_t operator[](uint8_t index) const { return regs[index]; }
};

class Memory {
  private:
    std::vector<uint8_t> data;
    Counters& ctrs;

    static constexpr uint32_t MEM_BASE = 0;

    size_t translate(uint32_t address, uint32_t pc, uint32_t width) const;
    void fail_loudly(uint32_t address, uint32_t pc, uint32_t width,
                     const char* reason) const;

  public:
    Memory(size_t size, Counters& _ctrs);

    uint32_t fetch32(uint32_t address);
    uint8_t read8(uint32_t address, uint32_t pc);
    uint32_t read32(uint32_t address, uint32_t pc);

    void write8(uint32_t address, uint32_t pc, uint8_t val);
    void write32(uint32_t address, uint32_t pc, uint32_t val);
};
class CPU {
    Counters ctrs;
    RegisterFile regs;
    Memory mem;
    uint32_t pc;
    bool halted;

  public:
    CPU(int _memSize);
    void step();
    void run();
    void load(std::vector<uint32_t> insVec, uint32_t startAddress = 0);
    friend void dump_state(const CPU& cpu);

    ExecResult execute_context(const Instruction& ins, int32_t& pcInc);
    // Temporary
    const Memory& getMemRef();
};
void dump_state(const CPU& cpu);

#endif // SYSTEM_H
