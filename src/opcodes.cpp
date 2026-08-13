/**
 * @file opcodes.h
 * @brief Implementing Opcodes ://
 */
#include "opcodes.h"
#include "system.h"
#include <iostream>
///@brief Anonymous namespace for enums to identify instructions from their
/// fields
namespace XYZ {

    /**
     * @brief RISC-V U-Type Opcodes
     */
    enum UOpcode : uint8_t {
        LUI = 0x37, ///< Load Upper Immediate: Loads 20-Bit Imm, bit
                    ///< left-shifted 12 bits,
        ///< directly to RD.

        AUIPC = 0x17 ///< Add Upper Immediate to PC: Adds 20-Bit Imm, bit
        ///< left-shifted 12 bits, to the address at the PC and
        ///< stores it to the RD.

    };

    /**
     * @brief RISC-V R-Type funct3 Values
     */
    enum RFunct3 : uint8_t {
        ADD_SUB = 0b000, ///< Add/Sub
        SLL = 0b001,     ///< Shift Left Logical
        SLT = 0b010,     ///< Set Less Than
        SLTU = 0b011,    ///< Shift Less Than Unsigned
        XOR = 0b100,     ///< Bitwise XOR
        SRL_SRA = 0b101, ///< Shift Right Logical / Shift Right Arithmetic
        OR = 0b110,      ///< Bitwise OR
        AND = 0b111      ///< Bitwise AND
    };

    /**
     * @brief RISC-V R-Type funct3 Values
     */
    enum RFunct7 : uint8_t {
        ADD = 0b0000000, ///< Add
        SUB = 0b0100000, ///< Sub

        SRL = 0b0000000, ///< Shift Right Logical
        SRA = 0b0100000, ///< Shift Right Arithmetic
    };

    /**
     * @brief RISC-V I-Type Opcodes
     */
    enum IOpcode : uint8_t {
        OP_IMM = 0b0010011,
        LOAD = 0b0000011,
        JALR = 0b1100111
    };

    enum IFunct3 : uint8_t {
        ADDI = 0b000,
        SLLI = 0b001,
        SLTI = 0b010,
        SLTIU = 0b011,
        XORI = 0b100,
        SRLI_SRAI = 0b101,
        ORI = 0b110,
        ANDI = 0b111,

        LB = 0b000,
        LH = 0b001,
        LW = 0b010,
        LBU = 0b100,
        LHU = 0b101,

    };

}; // namespace XYZ

using namespace XYZ;
auto machine_code(RegisterFile& rf, const Instruction& ins)
    -> std::optional<bool> {

    return std::visit(
        overloaded{
            [&](const RType& ins) -> std::optional<bool> {
                uint8_t funct3 = ins.get_funct3();

                uint8_t rs2 = ins.get_rs2();
                uint8_t rs1 = ins.get_rs1();
                uint8_t rd = ins.get_rd();

                switch (funct3) {
                case ADD_SUB: {
                    uint8_t funct7 = get_funct7(ins).value();
                    uint32_t temp = rf.read(rs2);
                    if (funct7 == SUB)
                        temp = static_cast<uint8_t>(-temp);
                    rf.write(rd, static_cast<uint32_t>(rf.read(rs1) + temp));
                    return true;
                }
                case SLL: {
                    rf.write(rd, rf.read(rs1) << (0x1F & rf.read(rs2)));
                    return true;
                }
                case SLT: {
                    rf.write(rd, (static_cast<int32_t>(rf.read(rs1)) <
                                  static_cast<int32_t>(rf.read(rs2))));
                    return true;
                }
                case SLTU: {
                    rf.write(rd, (rf.read(rs1) < rf.read(rs2)));
                    return true;
                }
                case XOR: {
                    std::cout << "RS1 " << rf.read(rs1) << '\n';
                    std::cout << "RS2 " << rf.read(rs2) << '\n';
                    rf.write(rd, rf.read(rs1) ^ rf.read(rs2));
                    return true;
                }
                case SRL_SRA: {
                    uint8_t funct7 = get_funct7(ins).value();
                    if (funct7 == SRA) {
                        rf.write(rd, static_cast<int32_t>(rf.read(rs1)) >>
                                         (0x1F & rf.read(rs2)));
                    } else {
                        rf.write(rd, rf.read(rs1) >> (0x1F & rf.read(rs2)));
                    }
                    return true;
                }
                case OR: {
                    rf.write(rd, rf.read(rs1) | rf.read(rs2));
                    return true;
                }
                case AND: {
                    rf.write(rd, rf.read(rs1) & rf.read(rs2));
                    return true;
                }
                default:
                    return std::nullopt;
                }
            },

            [&](const IType& ins) -> std::optional<bool> {
                uint8_t funct3 = ins.get_funct3();
                uint8_t rs1 = ins.get_rs1();
                uint8_t rd = ins.get_rd();
                int32_t imm = ins.get_imm();

                return true;
                switch (funct3) {
                case ADDI: {
                }
                case SLLI: {
                }
                case SLTI: {
                }
                case SLTIU: {
                }
                case XORI: {
                }
                case SRLI_SRAI: {
                }
                case ORI: {
                }
                case ANDI: {
                }
                default:
                    return std::nullopt;
                }
            },

            [&](const SType& ins) -> std::optional<bool> { return true; },
            [&](const BType& ins) -> std::optional<bool> { return true; },
            [&](const UType& ins) -> std::optional<bool> {
                auto opcode = RV32::base::get_opcode(ins.raw);
                switch (opcode) {

                case LUI: {
                    rf.write(ins.get_rd(), (+ins.get_imm()));
                    return true;
                }

                case AUIPC: {
                    rf.write(ins.get_rd(), (rf.read(0) + ins.get_imm()));
                    return true;
                }
                default:
                    return std::nullopt;
                }
            },
            [&](const JType& ins) -> std::optional<bool> { return true; }},
        ins);
}
