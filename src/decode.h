#ifndef DECODE_H
#define DECODE_H

#include <cstdint>
#include <optional>
#include <variant>

namespace RV32 {
    consteval auto getAndMask(int i, int j) -> uint32_t {
        int k = j - i + 1;
        uint32_t num = (1u << k) - 1;
        num <<= i;
        return num;
    }

    constexpr uint32_t OPCODE_MASK = getAndMask(0, 6);
    constexpr uint32_t RD_MASK = getAndMask(7, 11);
    constexpr uint32_t FUNCT3_MASK = getAndMask(12, 14);
    constexpr uint32_t RS1_MASK = getAndMask(15, 19);
    constexpr uint32_t RS2_MASK = getAndMask(20, 24);
    constexpr uint32_t FUNCT7_MASK = getAndMask(25, 31);

    namespace base {
        inline constexpr auto base_get_opcode(uint32_t ins) -> uint8_t {
            return (ins & OPCODE_MASK);
        }
        inline constexpr auto base_get_rd(uint32_t ins) -> uint8_t {
            return (ins & RD_MASK) >> 7;
        }
        inline constexpr auto base_get_funct3(uint32_t ins) -> uint8_t {
            return (ins & FUNCT3_MASK) >> 12;
        }
        inline constexpr auto base_get_rs1(uint32_t ins) -> uint8_t {
            return (ins & RS1_MASK) >> 15;
        }
        inline constexpr auto base_get_rs2(uint32_t ins) -> uint8_t {
            return (ins & RS2_MASK) >> 20;
        }
        inline constexpr auto base_get_funct7(uint32_t ins) -> uint8_t {
            return (ins & FUNCT7_MASK) >> 25;
        }
    } // namespace base
} // namespace RV32

struct RType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::base_get_rd(raw); }
    constexpr uint8_t get_funct3() const {
        return RV32::base::base_get_funct3(raw);
    }
    constexpr uint8_t get_rs1() const { return RV32::base::base_get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::base_get_rs2(raw); }
    constexpr uint8_t get_funct7() const {
        return RV32::base::base_get_funct7(raw);
    }
};

struct IType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::base_get_rd(raw); }
    constexpr uint8_t get_funct3() const {
        return RV32::base::base_get_funct3(raw);
    }
    constexpr uint8_t get_rs1() const { return RV32::base::base_get_rs1(raw); }
    constexpr int32_t get_imm() const {
        return static_cast<int32_t>(raw) >> 20;
    }
};

struct SType {
    uint32_t raw;
    constexpr uint8_t get_funct3() const {
        return RV32::base::base_get_funct3(raw);
    }
    constexpr uint8_t get_rs1() const { return RV32::base::base_get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::base_get_rs2(raw); }
    constexpr int32_t get_imm() const {
        return (static_cast<int32_t>(raw & 0xFE000000) >> 20) |
               RV32::base::base_get_rd(raw);
    }
};

struct BType {
    uint32_t raw;
    constexpr uint8_t get_funct3() const {
        return RV32::base::base_get_funct3(raw);
    }
    constexpr uint8_t get_rs1() const { return RV32::base::base_get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::base_get_rs2(raw); }
    constexpr int32_t get_imm() const {
        return (static_cast<int32_t>(raw & 0x80000000) >> 19) |
               ((RV32::base::base_get_rd(raw) & 0x01) << 11) |
               ((RV32::base::base_get_funct7(raw) & 0x3F) << 5) |
               (RV32::base::base_get_rd(raw) & 0x1E);
    }
};

struct UType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::base_get_rd(raw); }
    constexpr int32_t get_imm() const {
        return static_cast<int32_t>(raw & 0xFFFFF000);
    }
};

struct JType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::base_get_rd(raw); }
    constexpr int32_t get_imm() const {
        return (static_cast<int32_t>(raw & 0x80000000) >> 11) |
               static_cast<int32_t>((raw & 0xFF000) | ((raw >> 9) & 0x800) |
                                    ((raw >> 20) & 0x7FE));
    }
};

using Instruction = std::variant<RType, IType, SType, BType, UType, JType>;

auto decode(uint32_t raw_inst) -> Instruction;

// C++ Pattern Matching Engine
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

// Deduction Guide
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

auto get_opcode(const Instruction& ins) -> std::optional<uint8_t>;
auto get_rd(const Instruction& ins) -> std::optional<uint8_t>;
auto get_funct3(const Instruction& ins) -> std::optional<uint8_t>;
auto get_rs1(const Instruction& ins) -> std::optional<uint8_t>;
auto get_rs2(const Instruction& ins) -> std::optional<uint8_t>;
auto get_funct7(const Instruction& ins) -> std::optional<uint8_t>;
auto get_imm(const Instruction& ins) -> std::optional<uint8_t>;
#endif
