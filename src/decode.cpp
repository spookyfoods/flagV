#include "decode.h"
#include <stdexcept>

auto decode(uint32_t raw_inst) -> Instruction {
    uint8_t opcode = RV32::base::base_get_opcode(raw_inst);
    switch (opcode) {
    case 0x33:
        return RType{raw_inst};
    case 0x13:
    case 0x03:
    case 0x67:
    case 0x73:
    case 0x0F:
        return IType{raw_inst};
    case 0x23:
        return SType{raw_inst};
    case 0x63:
        return BType{raw_inst};
    case 0x37:
    case 0x17:
        return UType{raw_inst};
    case 0x6F:
        return JType{raw_inst};
    default:
        throw std::runtime_error("Invalid Opcode");
    }
}

auto get_rd(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{[](const RType& inst) -> std::optional<uint8_t> {
                       return inst.get_rd();
                   },
                   [](const IType& inst) -> std::optional<uint8_t> {
                       return inst.get_rd();
                   },
                   [](const UType& inst) -> std::optional<uint8_t> {
                       return inst.get_rd();
                   },
                   [](const JType& inst) -> std::optional<uint8_t> {
                       return inst.get_rd();
                   },

                   // Return std::nullopt for types without rd
                   [](const SType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   },
                   [](const BType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   }},
        ins);
}
auto get_opcode(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(overloaded{
                          [](const RType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                          [](const IType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                          [](const SType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                          [](const BType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                          [](const UType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                          [](const JType& inst) -> std::optional<uint8_t> {
                              return RV32::base::base_get_opcode(inst.raw);
                          },
                      },
                      ins);
}
auto get_funct3(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{[](const RType& inst) -> std::optional<uint8_t> {
                       return inst.get_funct3();
                   },
                   [](const IType& inst) -> std::optional<uint8_t> {
                       return inst.get_funct3();
                   },
                   [](const SType& inst) -> std::optional<uint8_t> {
                       return inst.get_funct3();
                   },
                   [](const BType& inst) -> std::optional<uint8_t> {
                       return inst.get_funct3();
                   },

                   // Return std::nullopt for types without rd
                   [](const UType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   },
                   [](const JType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   }},
        ins);
}
auto get_rs1(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{[](const RType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs1();
                   },
                   [](const IType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs1();
                   },
                   [](const SType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs1();
                   },
                   [](const BType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs1();
                   },

                   // Return std::nullopt for types without rd
                   [](const UType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   },
                   [](const JType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   }},
        ins);
}
auto get_rs2(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{[](const RType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs2();
                   },
                   [](const SType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs2();
                   },
                   [](const BType& inst) -> std::optional<uint8_t> {
                       return inst.get_rs2();
                   },

                   // Return std::nullopt for types without rd
                   [](const IType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   },
                   [](const UType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   },
                   [](const JType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   }},
        ins);
}
auto get_imm(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{[](const IType& inst) -> std::optional<uint8_t> {
                       return inst.get_imm();
                   },
                   [](const SType& inst) -> std::optional<uint8_t> {
                       return inst.get_imm();
                   },
                   [](const BType& inst) -> std::optional<uint8_t> {
                       return inst.get_imm();
                   },
                   [](const UType& inst) -> std::optional<uint8_t> {
                       return inst.get_imm();
                   },
                   [](const JType& inst) -> std::optional<uint8_t> {
                       return inst.get_imm();
                   },

                   // Return std::nullopt for types without rd
                   [](const RType& inst) -> std::optional<uint8_t> {
                       return std::nullopt;
                   }},
        ins);
}
auto get_funct7(const Instruction& ins) -> std::optional<uint8_t> {
    return std::visit(
        overloaded{
            [](const RType& inst) -> std::optional<uint8_t> {
                return inst.get_funct7();
            },
            [](const auto&) -> std::optional<uint8_t> { return std::nullopt; },
        },
        ins);
}
