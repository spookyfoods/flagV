#include "decode.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

// Conventions assumed by these tests:
//   * get_imm() returns a SIGN-EXTENDED int32_t.
//   * For I/S/B/J types get_imm() is the immediate/byte-offset in full
//     (B and J are already multiplied by 2, i.e. the low zero bit is present).
//   * For U type get_imm() is the immediate ALREADY SHIFTED left by 12,
//     so `lui x2, 0x12345` yields 0x12345000.
//   * For shift-immediates (slli/srli/srai) get_imm() returns the raw 12-bit
//     field, so srai carries the 0x400 marker bit. Swap to get_shamt() if
//     your decoder exposes one instead.
// Every encoding below was generated from the RV32I spec and cross-checked.

TEST_CASE("RV32 Compile-time Mask Generator", "[decode][mask]") {
    SECTION("Standard field masks") {
        STATIC_REQUIRE(RV32::getAndMask(0, 6) == 0x0000007F);
        STATIC_REQUIRE(RV32::getAndMask(7, 11) == 0x00000F80);
        STATIC_REQUIRE(RV32::getAndMask(12, 14) == 0x00007000);
        STATIC_REQUIRE(RV32::getAndMask(15, 19) == 0x000F8000);
        STATIC_REQUIRE(RV32::getAndMask(20, 24) == 0x01F00000);
        STATIC_REQUIRE(RV32::getAndMask(25, 31) == 0xFE000000);
    }

    SECTION("Single-bit and boundary ranges") {
        STATIC_REQUIRE(RV32::getAndMask(0, 0) == 0x00000001);
        STATIC_REQUIRE(RV32::getAndMask(7, 7) == 0x00000080);
        STATIC_REQUIRE(RV32::getAndMask(31, 31) == 0x80000000);
        STATIC_REQUIRE(RV32::getAndMask(20, 31) == 0xFFF00000);
        STATIC_REQUIRE(RV32::getAndMask(12, 31) == 0xFFFFF000);
    }

    SECTION("Named masks match the generator") {
        REQUIRE(RV32::OPCODE_MASK == 0x0000007F);
        REQUIRE(RV32::RD_MASK == 0x00000F80);
        REQUIRE(RV32::FUNCT3_MASK == 0x00007000);
        REQUIRE(RV32::RS1_MASK == 0x000F8000);
        REQUIRE(RV32::RS2_MASK == 0x01F00000);
        REQUIRE(RV32::FUNCT7_MASK == 0xFE000000);
    }

    SECTION("Masks are disjoint and cover the whole word") {
        REQUIRE((RV32::OPCODE_MASK | RV32::RD_MASK | RV32::FUNCT3_MASK |
                 RV32::RS1_MASK | RV32::RS2_MASK | RV32::FUNCT7_MASK) ==
                0xFFFFFFFF);
        REQUIRE((RV32::RD_MASK & RV32::FUNCT3_MASK) == 0);
        REQUIRE((RV32::RS1_MASK & RV32::RS2_MASK) == 0);
    }
}

TEST_CASE("R-Type Format Decoding", "[decode][format][rtype]") {
    SECTION("add x3, x1, x2") {
        RType r{0x002081B3};
        REQUIRE(r.get_rd() == 3);
        REQUIRE(r.get_funct3() == 0);
        REQUIRE(r.get_rs1() == 1);
        REQUIRE(r.get_rs2() == 2);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("sub x10, x11, x12") {
        RType r{0x40C58533};
        REQUIRE(r.get_rd() == 10);
        REQUIRE(r.get_funct3() == 0);
        REQUIRE(r.get_rs1() == 11);
        REQUIRE(r.get_rs2() == 12);
        REQUIRE(r.get_funct7() == 0x20);
    }

    SECTION("sll x31, x30, x29") {
        RType r{0x01DF1FB3};
        REQUIRE(r.get_rd() == 31);
        REQUIRE(r.get_funct3() == 1);
        REQUIRE(r.get_rs1() == 30);
        REQUIRE(r.get_rs2() == 29);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("sltu x1, x0, x22") {
        RType r{0x016030B3};
        REQUIRE(r.get_rd() == 1);
        REQUIRE(r.get_funct3() == 3);
        REQUIRE(r.get_rs1() == 0);
        REQUIRE(r.get_rs2() == 22);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("xor x7, x8, x9") {
        RType r{0x009443B3};
        REQUIRE(r.get_rd() == 7);
        REQUIRE(r.get_funct3() == 4);
        REQUIRE(r.get_rs1() == 8);
        REQUIRE(r.get_rs2() == 9);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("srl x2, x0, x21") {
        RType r{0x01505133};
        REQUIRE(r.get_rd() == 2);
        REQUIRE(r.get_funct3() == 5);
        REQUIRE(r.get_rs1() == 0);
        REQUIRE(r.get_rs2() == 21);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("sra x5, x0, x31 (same funct3 as srl, funct7 discriminates)") {
        RType r{0x41F052B3};
        REQUIRE(r.get_rd() == 5);
        REQUIRE(r.get_funct3() == 5);
        REQUIRE(r.get_rs1() == 0);
        REQUIRE(r.get_rs2() == 31);
        REQUIRE(r.get_funct7() == 0x20);
    }

    SECTION("and x8, x9, x10") {
        RType r{0x00A4F433};
        REQUIRE(r.get_rd() == 8);
        REQUIRE(r.get_funct3() == 7);
        REQUIRE(r.get_rs1() == 9);
        REQUIRE(r.get_rs2() == 10);
        REQUIRE(r.get_funct7() == 0);
    }

    SECTION("All zero operands (add x0, x0, x0)") {
        RType r{0x00000033};
        REQUIRE(r.get_rd() == 0);
        REQUIRE(r.get_funct3() == 0);
        REQUIRE(r.get_rs1() == 0);
        REQUIRE(r.get_rs2() == 0);
        REQUIRE(r.get_funct7() == 0);
    }
}

TEST_CASE("I-Type Format Decoding", "[decode][format][itype]") {
    SECTION("Positive immediate (addi x3, x1, 5)") {
        IType i{0x00508193};
        REQUIRE(i.get_rd() == 3);
        REQUIRE(i.get_funct3() == 0);
        REQUIRE(i.get_rs1() == 1);
        REQUIRE(i.get_imm() == 5);
    }

    SECTION("Negative immediate / sign-extension (addi x3, x1, -1)") {
        IType i{0xFFF08193};
        REQUIRE(i.get_rd() == 3);
        REQUIRE(i.get_funct3() == 0);
        REQUIRE(i.get_rs1() == 1);
        REQUIRE(i.get_imm() == -1);
    }

    SECTION("Maximum positive immediate (addi x1, x2, 2047)") {
        IType i{0x7FF10093};
        REQUIRE(i.get_rd() == 1);
        REQUIRE(i.get_rs1() == 2);
        REQUIRE(i.get_imm() == 2047);
    }

    SECTION("Minimum negative immediate (addi x1, x2, -2048)") {
        IType i{0x80010093};
        REQUIRE(i.get_rd() == 1);
        REQUIRE(i.get_rs1() == 2);
        REQUIRE(i.get_imm() == -2048);
    }

    SECTION("andi x31, x30, -16") {
        IType i{0xFF0F7F93};
        REQUIRE(i.get_rd() == 31);
        REQUIRE(i.get_funct3() == 7);
        REQUIRE(i.get_rs1() == 30);
        REQUIRE(i.get_imm() == -16);
    }

    SECTION("Shift immediate (slli x7, x8, 5)") {
        IType i{0x00541393};
        REQUIRE(i.get_rd() == 7);
        REQUIRE(i.get_funct3() == 1);
        REQUIRE(i.get_rs1() == 8);
        REQUIRE(i.get_imm() == 5); // shamt lives in the low 5 bits
    }

    SECTION("Arithmetic shift immediate (srai x7, x8, 5)") {
        IType i{0x40545393};
        REQUIRE(i.get_rd() == 7);
        REQUIRE(i.get_funct3() == 5);
        REQUIRE(i.get_rs1() == 8);
        REQUIRE(i.get_imm() == 0x405); // 0x400 marker | shamt of 5
    }

    SECTION("Load (lw x5, 16(x6))") {
        IType i{0x01032283};
        REQUIRE(i.get_rd() == 5);
        REQUIRE(i.get_funct3() == 2);
        REQUIRE(i.get_rs1() == 6);
        REQUIRE(i.get_imm() == 16);
    }

    SECTION("Load with negative offset (lbu x9, -4(x10))") {
        IType i{0xFFC54483};
        REQUIRE(i.get_rd() == 9);
        REQUIRE(i.get_funct3() == 4);
        REQUIRE(i.get_rs1() == 10);
        REQUIRE(i.get_imm() == -4);
    }

    SECTION("jalr x1, x2, -32") {
        IType i{0xFE0100E7};
        REQUIRE(i.get_rd() == 1);
        REQUIRE(i.get_funct3() == 0);
        REQUIRE(i.get_rs1() == 2);
        REQUIRE(i.get_imm() == -32);
    }
}

TEST_CASE("S-Type Format Decoding", "[decode][format][stype]") {
    SECTION("Positive offset (sw x2, 8(x1))") {
        SType s{0x0020A423};
        REQUIRE(s.get_funct3() == 2);
        REQUIRE(s.get_rs1() == 1);
        REQUIRE(s.get_rs2() == 2);
        REQUIRE(s.get_imm() == 8);
    }

    SECTION("Negative offset (sw x2, -2(x1))") {
        SType s{0xFE20AF23};
        REQUIRE(s.get_funct3() == 2);
        REQUIRE(s.get_rs1() == 1);
        REQUIRE(s.get_rs2() == 2);
        REQUIRE(s.get_imm() == -2);
    }

    SECTION("Maximum positive offset (sb x31, 2047(x30))") {
        SType s{0x7FFF0FA3};
        REQUIRE(s.get_funct3() == 0);
        REQUIRE(s.get_rs1() == 30);
        REQUIRE(s.get_rs2() == 31);
        REQUIRE(s.get_imm() == 2047);
    }

    SECTION("Minimum negative offset (sh x1, -2048(x2))") {
        SType s{0x80111023};
        REQUIRE(s.get_funct3() == 1);
        REQUIRE(s.get_rs1() == 2);
        REQUIRE(s.get_rs2() == 1);
        REQUIRE(s.get_imm() == -2048);
    }

    SECTION("Zero offset (sw x0, 0(x0))") {
        SType s{0x00002023};
        REQUIRE(s.get_funct3() == 2);
        REQUIRE(s.get_rs1() == 0);
        REQUIRE(s.get_rs2() == 0);
        REQUIRE(s.get_imm() == 0);
    }
}

TEST_CASE("B-Type Format Decoding", "[decode][format][btype]") {
    SECTION("Forward branch (beq x1, x2, 8)") {
        BType b{0x00208463};
        REQUIRE(b.get_funct3() == 0);
        REQUIRE(b.get_rs1() == 1);
        REQUIRE(b.get_rs2() == 2);
        REQUIRE(b.get_imm() == 8);
    }

    SECTION("Backward branch (beq x1, x2, -4)") {
        BType b{0xFE208EE3};
        REQUIRE(b.get_funct3() == 0);
        REQUIRE(b.get_rs1() == 1);
        REQUIRE(b.get_rs2() == 2);
        REQUIRE(b.get_imm() == -4);
    }

    SECTION("Maximum forward branch (bne x5, x6, 4094)") {
        BType b{0x7E629FE3};
        REQUIRE(b.get_funct3() == 1);
        REQUIRE(b.get_rs1() == 5);
        REQUIRE(b.get_rs2() == 6);
        REQUIRE(b.get_imm() == 4094);
    }

    SECTION("Maximum backward branch (blt x7, x8, -4096)") {
        BType b{0x8083C063};
        REQUIRE(b.get_funct3() == 4);
        REQUIRE(b.get_rs1() == 7);
        REQUIRE(b.get_rs2() == 8);
        REQUIRE(b.get_imm() == -4096);
    }

    SECTION(
        "Bit 11 of the immediate is the scrambled one (bgeu x30, x31, 2048)") {
        BType b{0x01FF70E3};
        REQUIRE(b.get_funct3() == 7);
        REQUIRE(b.get_rs1() == 30);
        REQUIRE(b.get_rs2() == 31);
        REQUIRE(b.get_imm() == 2048);
    }
}

TEST_CASE("U-Type Format Decoding", "[decode][format][utype]") {
    SECTION("lui x2, 0x12345") {
        UType u{0x12345137};
        REQUIRE(u.get_rd() == 2);
        REQUIRE(u.get_imm() == static_cast<int32_t>(0x12345000));
    }

    SECTION("auipc x1, 0xFFFFF (all immediate bits set)") {
        UType u{0xFFFFF097};
        REQUIRE(u.get_rd() == 1);
        REQUIRE(u.get_imm() == static_cast<int32_t>(0xFFFFF000));
    }

    SECTION("lui x31, 0x80000 (sign bit only)") {
        UType u{0x80000FB7};
        REQUIRE(u.get_rd() == 31);
        REQUIRE(u.get_imm() == static_cast<int32_t>(0x80000000));
    }

    SECTION("lui x0, 0") {
        UType u{0x00000037};
        REQUIRE(u.get_rd() == 0);
        REQUIRE(u.get_imm() == 0);
    }
}

// TEST_CASE("J-Type Format Decoding", "[decode][format][jtype]") {
//     SECTION("Forward jump (jal x0, 8)") {
//         JType j{0x0080006F};
//         REQUIRE(j.get_rd() == 0);
//         REQUIRE(j.get_imm() == 8);
//     }

//     SECTION("Backward jump (jal x0, -4)") {
//         JType j{0xFFDFF06F};
//         REQUIRE(j.get_rd() == 0);
//         REQUIRE(j.get_imm() == -4);
//     }

//     SECTION("Exercises imm[19:12] (jal x1, 4096)") {
//         JType j{0x000010EF};
//         REQUIRE(j.get_rd() == 1);
//         REQUIRE(j.get_imm() == 4096);
//     }

//     SECTION("Maximum forward jump (jal x31, 1048574)") {
//         JType j{0x7FFFFFEF};
//         REQUIRE(j.get_rd() == 31);
//         REQUIRE(j.get_imm() == 1048574);
//     }

//     SECTION("Maximum backward jump (jal x5, -1048576)") {
//         JType j{0x800002EF};
//         REQUIRE(j.get_rd() == 5);
//         REQUIRE(j.get_imm() == -1048576);
//     }
// }
