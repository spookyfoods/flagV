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
    /// @brief Total number of instructions executed by the CPU.
    uint64_t icount{0};
    /// @brief Total number of memory read operations.
    uint64_t loads{0};
    /// @brief Total number of memory write operations.
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
    explicit RegisterFile() = default;
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

    static constexpr uint32_t MEM_BASE = 0x0;

    void fail_loudly(uint32_t address, uint32_t pc, uint32_t width,
                     const char* reason) const;

  public:
    size_t translate(uint32_t address, uint32_t pc, uint32_t width) const;
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
     * @brief Reads 32-Bit(4 Bytes) Instruction from memory.
     * @param address The target in the memory, to read the instruction from.
     * @retval 32Bit-Machine-Instruction fetched from address, in Little-Endian.
     */
    uint32_t fetch32(uint32_t address);

    template <int sz>
        requires(sz % 8 == 0 && sz > 0)
    void write(uint32_t address, uint32_t pc, uint32_t val) {
        uint32_t width = sz / 8;
        size_t offset = translate(address, pc, width);
        ctrs.inc_stores();
        for (size_t i = 0; i < width; i++) {
            data[offset + i] = static_cast<uint8_t>(0x000000FF & val);
            val >>= 8;
        }
    }

    friend class CPU;
};

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
template <> void Memory::write<8>(uint32_t address, uint32_t pc, uint32_t val);

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
template <> void Memory::write<16>(uint32_t address, uint32_t pc, uint32_t val);

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
template <> void Memory::write<32>(uint32_t address, uint32_t pc, uint32_t val);
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

    /**
     * @brief Loads a vector of 32-Bit raw instructions, at startAddress
     * @param[in] insVec A vector of 32-Bit Raw Instructions
     * @param[in] startAddress Address in memory to start loading instructions
     * @note The vector contains RAW instructions, since they are to be loaded
     * into the memory; they will be processed after being fetched, later in the
     * lifecycle
     */
    void load(std::vector<uint32_t> insVec, uint32_t startAddress = 0);

    ExecResult execute_context(const Instruction& ins, int32_t& pcInc);

    friend void dump_state(const CPU& cpu);
    friend struct CpuProbe;
};
void dump_state(const CPU& cpu);

#endif // SYSTEM_H
