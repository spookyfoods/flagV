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
    explicit Memory(size_t size, Counters& _ctrs);
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    /**
     * @brief Reads 8-Bit(1-Byte) from memory.
     * @param address The target in the memory, to read the byte from.
     * @retval 1-Byte Content fetched from address.
     */
    uint8_t read8(uint32_t address, uint32_t pc);

    /**
     * @brief Writes 8-Bit(1-Byte) value to memory.
     * @param[in] address The target memory address to write to.
     * @param[in] pc The current Program Counter(logging/debugging).
     * @param[in] val The value of the byte to write to the address
     * @note **Execution Demonstration:**
     * @code
     *    // State BEFORE execution:
     *    // Memory at [0x1000] = 0x00000000
     *
     *    write8(0x1000, 0x8004, 0x12);
     *
     *    // State AFTER execution:
     *    // Memory at [0x1000] = 12
     * @endcode
     */
    void write8(uint32_t address, uint32_t pc, uint8_t val);

    /**
     * @brief Reads a 16-Bit(2 Bytes) value from memory, in Little-Endian
     * format.
     * @param[in] address The target memory address to read from.
     * @param[in] pc The current Program Counter(logging/debugging).
     * @return The 16-bit half-word reassembled from the two bytes at the
     * address.
     * @note **Execution Demonstration:**
     * @code
     *    // State BEFORE execution:
     *    // Memory at [0x1000] = 12'AB (Stored in Little Endian Format)
     *
     *    uint32_t val = read32(0x1000, 0x8004);
     *
     *    // State AFTER execution:
     *    // val = 0xAB12 (Lowest address becomes the least significant
     * byte)
     * @endcode
     */
    uint16_t read16(uint32_t address, uint32_t pc);
    /**
     * @brief Writes 16-Bit(2 Bytes) value to memory, in Little-Endian format.
     * @param[in] address The target memory address to write to.
     * @param[in] pc The current Program Counter(logging/debugging).
     * @param[in] val The value of the word to write to the address
     * @note **Execution Demonstration:**
     * @code
     *    // State BEFORE execution:
     *    // Memory at [0x1000] = 0x00000000
     *
     *    write32(0x1000, 0x8004, 0xABCD);
     *
     *    // State AFTER execution:
     *    // Memory at [0x1000] = CD'AB (Written in Little Endian Format)
     * @endcode
     */
    void write16(uint32_t address, uint32_t pc, uint16_t val);
    /**
     * @brief Reads 32-Bit(4 Bytes) Instruction from memory.
     * @param address The target in the memory, to read the instruction from.
     * @retval 32Bit-Machine-Instruction fetched from address, in Little-Endian.
     */
    uint32_t fetch32(uint32_t address);
    /**
     * @brief Reads a 32-Bit(4 Bytes) value from memory, in Little-Endian
     * format.
     * @param[in] address The target memory address to read from.
     * @param[in] pc The current Program Counter(logging/debugging).
     * @return The 32-bit word reassembled from the four bytes at the address.
     * @note **Execution Demonstration:**
     * @code
     *    // State BEFORE execution:
     *    // Memory at [0x1000] = 12'EF'CD'AB (Stored in Little Endian Format)
     *
     *    uint32_t val = read32(0x1000, 0x8004);
     *
     *    // State AFTER execution:
     *    // val = 0xABCDEF12 (Lowest address becomes the least significant
     * byte)
     * @endcode
     */
    uint32_t read32(uint32_t address, uint32_t pc);
    /**
     * @brief Writes 32-Bit(4 Bytes) value to memory, in Little-Endian format.
     * @param[in] address The target memory address to write to.
     * @param[in] pc The current Program Counter(logging/debugging).
     * @param[in] val The value of the word to write to the address
     * @note **Execution Demonstration:**
     * @code
     *    // State BEFORE execution:
     *    // Memory at [0x1000] = 0x00000000
     *
     *    write32(0x1000, 0x8004, 0xABCDEF12);
     *
     *    // State AFTER execution:
     *    // Memory at [0x1000] = 12'EF'CD'AB (Written in Little Endian Format)
     * @endcode
     */
    void write32(uint32_t address, uint32_t pc, uint32_t val);
};
class CPU {
    Counters ctrs;
    RegisterFile regs;
    Memory mem;
    uint32_t pc;
    ExecResult result;

  public:
    CPU(int _memSize);
    void step();
    void run();
    void load(std::vector<uint32_t> insVec, uint32_t startAddress = 0);
    friend void dump_state(const CPU& cpu);

    ExecResult execute_context(const Instruction& ins, int32_t& pcInc);

    // Testing
    Memory& getMemRef();
    friend struct CpuProbe;
};
void dump_state(const CPU& cpu);

#endif // SYSTEM_H
