/**
 * @file opcodes.h
 * @brief Implementing Opcodes ://
 */
#include "config.h"
#include "system.h"
#include "trace.h"
#include <iostream>
#include <print>
///@brief Anonymous namespace for enums to identify instructions from their
/// fields
namespace FIELD_CONSTANTS {

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
        // Arithmetic Related Instructions
        OP_IMM = 0b0010011,

        // Memory Instructions
        LOAD = 0b0000011,

        // Control Instructions
        JALR = 0b1100111,

        // ECALL_EBREAK
        ECALL_EBREAK = 0b1110011
    };

    /**
     * @brief RISC-V I-Type funct3 Values
     */
    enum IFunct3 : uint8_t {
        // Arithmetic Related Instructions
        ADDI = 0b000,
        SLLI = 0b001,
        SLTI = 0b010,
        SLTIU = 0b011,
        XORI = 0b100,
        SRLI_SRAI = 0b101,
        ORI = 0b110,
        ANDI = 0b111,

        // Memory Instructions
        LB = 0b000,
        LH = 0b001,
        LW = 0b010,
        LBU = 0b100,
        LHU = 0b101,

    };

    /**
     * @brief RISC-V S-Type funct3 Values
     */
    enum SFunct3 : uint8_t {
        SB = 0b000,
        SH = 0b001,
        SW = 0b010,

    };

    /**
     * @brief RISC-V B-Type funct3 Values
     */
    enum BFunct3 : uint8_t {
        BEQ = 0b000,
        BNE = 0b001,
        BLT = 0b100,
        BLTU = 0b110,
        BGE = 0b101,
        BGEU = 0b111,
    };

    /**
     * @brief RISC-V U-Type Opcodes
     */
    enum UOpcode : uint8_t {
        LUI = 0x37, ///< Load Upper Immediate: Loads 20-Bit Imm, bit
                    ///< left-shifted 12 bits,
        ///< directly to RD."

        AUIPC = 0x17 ///< Add Upper Immediate to PC: Adds 20-Bit Imm, bit
        ///< left-shifted 12 bits, to the address at the PC and
        ///< stores it to the RD.

    };

}; // namespace FIELD_CONSTANTS

using namespace FIELD_CONSTANTS;
ExecResult CPU::execute_context(const Instruction& instruction,
                                int32_t& pcInc) {

    return std::visit(
        overloaded{
            [&](const RType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);

                uint8_t funct3 = ins.get_funct3();
                uint8_t rs2 = ins.get_rs2();
                uint8_t rs1 = ins.get_rs1();
                uint8_t rd = ins.get_rd();

                switch (funct3) {
                case ADD_SUB: {
                    uint8_t funct7 = ins.get_funct7();
                    uint32_t temp = regs.read(rs2);
                    switch (funct7) {
                    case ADD:
                        break;
                    case SUB:
                        temp = static_cast<uint32_t>(-temp);
                        break;
                    default:
                        return ExecResult::Fault;
                    }
                    regs.write(rd,
                               static_cast<uint32_t>(regs.read(rs1) + temp));
                    return ExecResult::Continue;
                }
                case SLL: {
                    regs.write(rd, regs.read(rs1) << (0x1F & regs.read(rs2)));
                    return ExecResult::Continue;
                }
                case SLT: {
                    regs.write(rd, (static_cast<int32_t>(regs.read(rs1)) <
                                    static_cast<int32_t>(regs.read(rs2))));
                    return ExecResult::Continue;
                }
                case SLTU: {
                    regs.write(rd, (regs.read(rs1) < regs.read(rs2)));
                    return ExecResult::Continue;
                }
                case XOR: {
                    regs.write(rd, regs.read(rs1) ^ regs.read(rs2));
                    return ExecResult::Continue;
                }
                case SRL_SRA: {
                    uint8_t funct7 = ins.get_funct7();
                    switch (funct7) {
                    case SRA: {
                        regs.write(rd, static_cast<int32_t>(regs.read(rs1)) >>
                                           (0x1F & regs.read(rs2)));
                        break;
                    }
                    case SRL: {
                        regs.write(rd,
                                   regs.read(rs1) >> (0x1F & regs.read(rs2)));
                        break;
                    }
                    default:
                        return ExecResult::Fault;
                    }
                    return ExecResult::Continue;
                }
                case OR: {
                    regs.write(rd, regs.read(rs1) | regs.read(rs2));
                    return ExecResult::Continue;
                }
                case AND: {
                    regs.write(rd, regs.read(rs1) & regs.read(rs2));
                    return ExecResult::Continue;
                }
                default:
                    return ExecResult::Fault;
                }
            },

            [&](const IType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);
                uint8_t funct3 = ins.get_funct3();
                uint8_t rs1 = ins.get_rs1();
                uint8_t rd = ins.get_rd();
                int32_t imm = ins.get_imm();
                auto opcode = RV32::base::get_opcode(ins.raw);

                switch (opcode) {
                case OP_IMM: {
                    switch (funct3) {
                    case ADDI: {
                        regs.write(rd,
                                   static_cast<uint32_t>(regs.read(rs1) + imm));
                        return ExecResult::Continue;
                    }
                    case (SLLI): {
                        regs.write(rd, regs.read(rs1) << (imm & 0x1F));
                        return ExecResult::Continue;
                    }
                    case (SLTI): {
                        regs.write(
                            rd,
                            static_cast<int32_t>(regs.read(rs1)) < imm ? 1 : 0);
                        return ExecResult::Continue;
                    }
                    case (SLTIU): {
                        regs.write(rd,
                                   regs.read(rs1) < static_cast<uint32_t>(imm)
                                       ? 1
                                       : 0);
                        return ExecResult::Continue;
                    }
                    case (XORI): {
                        regs.write(rd, regs.read(rs1) ^ imm);
                        return ExecResult::Continue;
                    }
                    case (SRLI_SRAI): {
                        if (0x400 & imm) {
                            regs.write(rd,
                                       static_cast<int32_t>(regs.read(rs1)) >>
                                           (imm & 0x1F));
                        } else {
                            regs.write(rd,
                                       static_cast<uint32_t>(regs.read(rs1)) >>
                                           (imm & 0x1F));
                        }
                        return ExecResult::Continue;
                    }
                    case (ORI): {
                        regs.write(rd, regs.read(rs1) | imm);
                        return ExecResult::Continue;
                    }
                    case (ANDI): {
                        regs.write(rd, regs.read(rs1) & imm);
                        return ExecResult::Continue;
                    }
                    default:
                        return ExecResult::Fault;
                    }
                }
                case LOAD: {
                    switch (funct3) {
                    case (LB): {
                        int32_t byte = static_cast<int8_t>(
                            mem.read8(regs.read(rs1) + imm, pc));

                        regs.write(rd, byte);

                        return ExecResult::Continue;
                    }
                    case (LBU): {
                        auto byte = static_cast<uint32_t>(
                            mem.read8(regs.read(rs1) + imm, pc));
                        regs.write(rd, byte);

                        return ExecResult::Continue;
                    }
                    case (LH): {
                        int32_t word = static_cast<int16_t>(
                            mem.read16(regs.read(rs1) + imm, pc));
                        regs.write(rd, word);
                        return ExecResult::Continue;
                    }
                    case (LHU): {
                        auto word = static_cast<uint32_t>(
                            mem.read16(regs.read(rs1) + imm, pc));
                        regs.write(rd, word);
                        return ExecResult::Continue;
                    }
                    case (LW): {
                        auto word = mem.read32(regs.read(rs1) + imm, pc);
                        regs.write(rd, word);
                        return ExecResult::Continue;
                    }
                    default:
                        return ExecResult::Fault;
                    }
                }
                case JALR: {
                    std::cerr << "JALR NOT IMPLEMENTED YET";
                    return ExecResult::Fault;
                }
                case ECALL_EBREAK: {
                    auto imm = ins.get_imm();
                    if (imm == 0) {
                        // ecall
                        auto a7 = regs.read(17);
                        switch (a7) {
                        case 1: {
                            std::println("{}",
                                         static_cast<int32_t>(regs.read(10)));
                            return ExecResult::Continue;
                        }
                        case 93: {
                            return ExecResult::Halt;
                        }
                        default:
                            return ExecResult::Fault;
                        }

                    } else if (imm == 1) {
                        // ebreak
                        return ExecResult::Halt;
                    } else {
                        return ExecResult::Fault;
                    }
                }
                default:
                    return ExecResult::Fault;
                }
            },

            [&](const SType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);
                auto funct3 = ins.get_funct3();
                auto imm = ins.get_imm();
                auto rs1 = ins.get_rs1();
                auto rs2 = ins.get_rs2();

                switch (funct3) {
                case (SB): {
                    uint32_t addrToWrite = regs.read(rs1) + imm;
                    mem.write<8>(addrToWrite, pc, 0x000000FF & regs.read(rs2));
                    return ExecResult::Continue;
                }
                case (SH): {
                    uint32_t addrToWrite = regs.read(rs1) + imm;
                    mem.write<16>(addrToWrite, pc, 0x0000FFFF & regs.read(rs2));
                    return ExecResult::Continue;
                }
                case (SW): {
                    uint32_t addrToWrite = regs.read(rs1) + imm;
                    mem.write<32>(addrToWrite, pc, regs.read(rs2));
                    return ExecResult::Continue;
                }
                default:
                    return ExecResult::Fault;
                }
            },
            [&](const BType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);
                auto funct3 = ins.get_funct3();
                auto rs1 = ins.get_rs1();
                auto rs2 = ins.get_rs2();
                auto imm = ins.get_imm();
                bool take_branch = false;

                switch (funct3) {
                case BEQ:
                    take_branch = (regs.read(rs1) == regs.read(rs2));
                    break;
                case BNE:
                    take_branch = (regs.read(rs1) != regs.read(rs2));
                    break;
                case BLT:
                    take_branch = static_cast<int32_t>(regs.read(rs1)) <
                                  static_cast<int32_t>(regs.read(rs2));
                    break;
                case BGE:
                    take_branch = static_cast<int32_t>(regs.read(rs1)) >=
                                  static_cast<int32_t>(regs.read(rs2));
                    break;
                case BLTU:
                    take_branch = (regs.read(rs1) < regs.read(rs2));
                    break;
                case BGEU:
                    take_branch = (regs.read(rs1) >= regs.read(rs2));
                    break;
                }
                if (take_branch) {
                    pcInc = imm;
                }
                return ExecResult::Continue;
            },
            [&](const UType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);
                auto opcode = RV32::base::get_opcode(ins.raw);
                switch (opcode) {

                case LUI: {
                    regs.write(ins.get_rd(), (+ins.get_imm()));
                    return ExecResult::Continue;
                }

                case AUIPC: {
                    regs.write(ins.get_rd(), pc + ins.get_imm());
                    return ExecResult::Continue;
                }
                default:
                    return ExecResult::Fault;
                }
            },
            [&](const JType& ins) -> ExecResult {
                if constexpr (config::EXECUTE_CONTEXT_VERBOSE)
                    trace(ins, this);
                auto rd = ins.get_rd();
                auto imm = ins.get_imm();
                regs.write(rd, pc + 4);
                pcInc = imm;

                return ExecResult::Continue;
            },
            [&](const auto&) -> ExecResult {
                std::cout << "HORRID ERROR: No Instruction Type Match inside "
                             "Execute Context\n"
                          << std::endl;
                return ExecResult::Fault;
            }},
        instruction);
}
