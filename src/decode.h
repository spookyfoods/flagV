/**
 * @file decode.h
 * @brief Bit-field extraction for RV32 instruction words.
 */
#ifndef DECODE_H
#define DECODE_H

#include <cstdint>
#include <optional>
#include <variant>

// ANCHOR-START: RV32
/// @brief RV32 base ISA decoding primitives.
namespace RV32 {
    /**
     * @brief Generates a contiguous 32-Bit Mask for bit ranges [i,j],
     * inclusive.
     * @param i 1s filling, starting point
     * @param j 1s filling, finishing point
     * @return 32-bit unsigned integer with bits [i, j] set to 1.
     * @code{.cpp}
     * uint32_t sample_mask = getAndMask(12, 25);
     * //returns -> 0b0000'0011'1111'1111'1111'0000'0000'0000
     * @endcode
     * @note Evaluated STRICTLY at compile-time
     * */
    consteval auto getAndMask(int i, int j) -> uint32_t {
        int k = j - i + 1;
        uint32_t num = (1u << k) - 1;
        num <<= i;
        return num;
    }
    /**
     * @name Basic RV32 instruction field bit-masks
     * @{
     */
    constexpr uint32_t OPCODE_MASK = getAndMask(0, 6);   ///< [6:0]
    constexpr uint32_t RD_MASK = getAndMask(7, 11);      ///< [11:7]
    constexpr uint32_t FUNCT3_MASK = getAndMask(12, 14); ///< [14:12]
    constexpr uint32_t RS1_MASK = getAndMask(15, 19);    ///< [19:15]
    constexpr uint32_t RS2_MASK = getAndMask(20, 24);    ///< [24:20]
    constexpr uint32_t FUNCT7_MASK = getAndMask(25, 31); ///< [31:25]
    /** @}*/

    /// @brief Base Namespace, only to stress the extractors are base and not
    /// instruction specifc
    namespace base {
        /**
         * @name Base Instruction Field Extractors
         * Common behavior across all extractors:
         * @note The returned masked value is BOTH consumption-ready and in its
         * numerically accurate unsigned integer form without padding.
         * @{
         */

        /// @brief Extracts the operation code (opcode).
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked opcode value as an unsigned 8-bit integer.
        inline constexpr auto get_opcode(uint32_t ins) -> uint8_t {
            return (ins & OPCODE_MASK);
        }

        /// @brief Extracts the destination register (rd).
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked rd register index as an unsigned 8-bit integer.
        inline constexpr auto get_rd(uint32_t ins) -> uint8_t {
            return (ins & RD_MASK) >> 7;
        }

        /// @brief Extracts the funct3 field.
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked funct3 as an unsigned 8-bit integer.
        inline constexpr auto get_funct3(uint32_t ins) -> uint8_t {
            return (ins & FUNCT3_MASK) >> 12;
        }

        /// @brief Extracts the source register 1 (rs1).
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked rs1 as an unsigned 8-bit integer.
        inline constexpr auto get_rs1(uint32_t ins) -> uint8_t {
            return (ins & RS1_MASK) >> 15;
        }

        /// @brief Extracts the source register 2 (rs2).
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked rs2 as an unsigned 8-bit integer.
        inline constexpr auto get_rs2(uint32_t ins) -> uint8_t {
            return (ins & RS2_MASK) >> 20;
        }

        /// @brief Extracts the funct7 field.
        /// @param ins The raw 32-bit instruction word to parse.
        /// @return The masked funct7 as an unsigned 8-bit integer.
        inline constexpr auto get_funct7(uint32_t ins) -> uint8_t {
            return (ins & FUNCT7_MASK) >> 25;
        }
        /** @} */
    } // namespace base

} // namespace RV32
// ANCHOR-END: RV32

/**
 * @defgroup formats RV32 Instruction Formats
 * @brief Typed views over a raw instruction word, one per RISC-V encoding.
 *
 * Each type wraps the undecoded word and exposes only the fields its
 * encoding actually defines. Field accessors delegate to RV32::base;
 * the immediate accessors perform format-specific reassembly and sign
 * extension.
 * @{
 */
struct RType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::get_rd(raw); }
    constexpr uint8_t get_funct3() const { return RV32::base::get_funct3(raw); }
    constexpr uint8_t get_rs1() const { return RV32::base::get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::get_rs2(raw); }
    constexpr uint8_t get_funct7() const { return RV32::base::get_funct7(raw); }
};

struct IType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::get_rd(raw); }
    constexpr uint8_t get_funct3() const { return RV32::base::get_funct3(raw); }
    constexpr uint8_t get_rs1() const { return RV32::base::get_rs1(raw); }

    /**
     * @brief Extracts bits [ 30 , 20 ], and bit shifts into standard 32-bit
     * uint format, the imm field is contiguous, so extraction is simple.
     *
     * | Instruction's bits | Immediate bits |
     * | ---------------- | -------------- |
     * | `[31:20]`         | `[11:0]`        |
     *
     * @return 32-Bit Unsigned Integer
     * @note The returned masked value is BOTH consumption-ready and in its
     * numerically accurate unsigned integer form without padding.
     */
    constexpr int32_t get_imm() const {
        return static_cast<int32_t>(raw) >> 20;
    }
};

struct SType {
    uint32_t raw;
    constexpr uint8_t get_funct3() const { return RV32::base::get_funct3(raw); }
    constexpr uint8_t get_rs1() const { return RV32::base::get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::get_rs2(raw); }

    /**
     * @brief Extracts and merges 2 disjoint set of bits, shifts into standard
     * 32-bit int format, in SType instructions the immediate serves as a
     * positive or negative offset, relative to the addreess stored in rs1
     *
     * | Instruction's bits | Immediate bits |
     * | ---------------- | -------------- |
     * | `[31:25]`         | `[11:5]`        |
     * | `[11:7]`         | `[4:0]`        |
     *
     * @return 32-Bit Signed Integer
     * @note The returned masked value is BOTH consumption-ready and in its
     * numerically accurate unsigned integer form without padding.
     */
    constexpr int32_t get_imm() const {
        return (static_cast<int32_t>(raw & 0xFE000000) >> 20) |
               RV32::base::get_rd(raw);
    }
};

struct BType {
    uint32_t raw;
    constexpr uint8_t get_funct3() const { return RV32::base::get_funct3(raw); }
    constexpr uint8_t get_rs1() const { return RV32::base::get_rs1(raw); }
    constexpr uint8_t get_rs2() const { return RV32::base::get_rs2(raw); }

    /**
     * @brief Reassembles and sign-extends the 13-bit branch offset.
     *
     * The B-format immediate is scattered across four disjoint ranges and
     * omits bit 0, which is implicitly zero (offsets are 2-byte aligned):
     *
     * | Instruction bits | Immediate bits |
     * | ---------------- | -------------- |
     * | `[31]`           | `[12]`         |
     * | `[7]`            | `[11]`         |
     * | `[30:25]`        | `[10:5]`       |
     * | `[11:8]`         | `[4:1]`        |
     *
     * @return The byte offset, sign-extended to 32 bits, in `[-4096, 4094]`.
     * @note The result is always even and is relative to the branch's own
     *       address, not the next instruction.
     */
    constexpr int32_t get_imm() const {
        return
            // Extracts Inst Bit 31 <=> 12
            (static_cast<int32_t>(raw & 0x80000000) >> 19) |

            // Extracts [7] <=> [11]
            ((RV32::base::get_rd(raw) & 0x01) << 11) |

            // Extracts [30:25] <=> [10:5]
            ((RV32::base::get_funct7(raw) & 0x3F) << 5) |

            // Extracts [11:8] <=> [4:1]
            (RV32::base::get_rd(raw) & 0x1E);
    }
};

struct UType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::get_rd(raw); }
    constexpr int32_t get_imm() const {
        return static_cast<int32_t>(raw & 0xFFFFF000);
    }
};

struct JType {
    uint32_t raw;
    constexpr uint8_t get_rd() const { return RV32::base::get_rd(raw); }
    constexpr int32_t get_imm() const {
        return (static_cast<int32_t>(raw & 0x80000000) >> 11) |
               static_cast<int32_t>((raw & 0xFF000) | ((raw >> 9) & 0x800) |
                                    ((raw >> 20) & 0x7FE));
    }
};
/** @} */

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
auto get_imm(const Instruction& ins) -> std::optional<int32_t>;
#endif
