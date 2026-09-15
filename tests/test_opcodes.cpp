#include "decode.h"
#include "system.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

// Requires `friend struct CpuProbe;` inside CPU.
struct CpuProbe {
    static RegisterFile& regs(CPU& c) noexcept { return c.regs; }
    static const RegisterFile& regs(const CPU& c) noexcept { return c.regs; }
    static uint32_t& pc(CPU& c) noexcept { return c.pc; }
    static ExecResult& result(CPU& c) noexcept { return c.result; }
    static Memory& mem(CPU& c) noexcept { return c.mem; }
};

namespace {

    // Every opcode encoded as `op x5, x1, x2`; only the operands vary.
    constexpr uint32_t ADD = 0x002082B3, SUB = 0x402082B3;
    constexpr uint32_t SLL = 0x002092B3, SLT = 0x0020A2B3;
    constexpr uint32_t SLTU = 0x0020B2B3, XOR = 0x0020C2B3;
    constexpr uint32_t SRL = 0x0020D2B3, SRA = 0x4020D2B3;
    constexpr uint32_t OR = 0x0020E2B3, AND = 0x0020F2B3;

    constexpr int MEM_SIZE = 1 << 10;

    void exec(CPU& c, uint32_t word) {
        int32_t pcInc = 0;
        c.execute_context(decode(word), pcInc);
    }

    /// Seeds x1/x2, executes, returns x5.
    uint32_t alu(CPU& c, uint32_t word, uint32_t rs1_val, uint32_t rs2_val) {
        CpuProbe::regs(c).write(1, rs1_val);
        CpuProbe::regs(c).write(2, rs2_val);
        exec(c, word);
        return CpuProbe::regs(c).read(5);
    }

    /// Seeds x1, executes an OP-IMM word (always `op x5, x1, imm`), returns x5.
    /// The immediate lives inside the word, so each case carries its own
    /// encoding rather than sharing a named constant.
    uint32_t alui(CPU& c, uint32_t word, uint32_t rs1_val) {
        CpuProbe::regs(c).write(1, rs1_val);
        exec(c, word);
        return CpuProbe::regs(c).read(5);
    }

} // namespace

TEST_CASE("R-Type ALU", "[cpu][execute][rtype]") {
    CPU c(MEM_SIZE);

    SECTION("ADD_SUB (0b000)") {
        REQUIRE(alu(c, ADD, 5, 3) == 8);
        REQUIRE(alu(c, ADD, 5, 0xFFFFFFFB) == 0);          // 5 + (-5)
        REQUIRE(alu(c, ADD, 0x7FFFFFFF, 1) == 0x80000000); // wraps, no trap
        REQUIRE(alu(c, ADD, 0xFFFFFFFF, 1) == 0);
        REQUIRE(alu(c, SUB, 8, 3) == 5);
        REQUIRE(alu(c, SUB, 3, 8) == 0xFFFFFFFB);
        REQUIRE(alu(c, SUB, 0, 1) == 0xFFFFFFFF);
        REQUIRE(alu(c, SUB, 7, 7) == 0);
        // Same funct3: only funct7 separates them.
        REQUIRE(alu(c, ADD, 20, 6) == 26);
        REQUIRE(alu(c, SUB, 20, 6) == 14);
    }

    SECTION("SLL (0b001)") {
        REQUIRE(alu(c, SLL, 1, 4) == 16);
        REQUIRE(alu(c, SLL, 1, 31) == 0x80000000);
        REQUIRE(alu(c, SLL, 0x80000000, 1) == 0);
        // shamt is rs2 & 0x1F; a shift of 32 must not be UB.
        REQUIRE(alu(c, SLL, 1, 32) == 1);
        REQUIRE(alu(c, SLL, 1, 33) == 2);
        REQUIRE(alu(c, SLL, 0xF, 36) == 0xF0);
    }

    SECTION("SLT (0b010) is signed") {
        REQUIRE(alu(c, SLT, 1, 2) == 1);
        REQUIRE(alu(c, SLT, 2, 1) == 0);
        REQUIRE(alu(c, SLT, 7, 7) == 0);
        REQUIRE(alu(c, SLT, 0xFFFFFFFF, 1) == 1); // -1 < 1
        REQUIRE(alu(c, SLT, 1, 0xFFFFFFFF) == 0);
        REQUIRE(alu(c, SLT, 0x80000000, 0x7FFFFFFF) == 1);
    }

    SECTION("SLTU (0b011) is unsigned") {
        REQUIRE(alu(c, SLTU, 1, 2) == 1);
        REQUIRE(alu(c, SLTU, 7, 7) == 0);
        REQUIRE(alu(c, SLTU, 0xFFFFFFFF, 1) == 0); // cf. SLT above
        REQUIRE(alu(c, SLTU, 1, 0xFFFFFFFF) == 1);
        REQUIRE(alu(c, SLTU, 0x7FFFFFFF, 0x80000000) == 1);
    }

    SECTION("XOR (0b100)") {
        REQUIRE(alu(c, XOR, 0b1100, 0b1010) == 0b0110);
        REQUIRE(alu(c, XOR, 0x12345678, 0xFFFFFFFF) == 0xEDCBA987);
        REQUIRE(alu(c, XOR, 0x12345678, 0x12345678) == 0);
    }

    SECTION("SRL_SRA (0b101)") {
        REQUIRE(alu(c, SRL, 16, 4) == 1);
        REQUIRE(alu(c, SRL, 0x80000000, 1) == 0x40000000); // zero-fill
        REQUIRE(alu(c, SRL, 0xFFFFFFFF, 31) == 1);
        REQUIRE(alu(c, SRA, 0x80000000, 1) == 0xC0000000); // sign-fill
        REQUIRE(alu(c, SRA, 0xFFFFFFF0, 4) == 0xFFFFFFFF); // -16 >> 4 == -1
        REQUIRE(alu(c, SRA, 0x7FFFFFFF, 1) == 0x3FFFFFFF); // matches SRL
        // Same funct3: only funct7 separates them.
        REQUIRE(alu(c, SRL, 0x80000000, 4) == 0x08000000);
        REQUIRE(alu(c, SRA, 0x80000000, 4) == 0xF8000000);
        REQUIRE(alu(c, SRL, 16, 33) == 8);
        REQUIRE(alu(c, SRA, 0xFFFFFFF0, 36) == 0xFFFFFFFF);
    }

    SECTION("OR (0b110)") {
        REQUIRE(alu(c, OR, 0b1100, 0b1010) == 0b1110);
        REQUIRE(alu(c, OR, 0x12345678, 0) == 0x12345678);
        REQUIRE(alu(c, OR, 0xF0F0F0F0, 0x0F0F0F0F) == 0xFFFFFFFF);
    }

    SECTION("AND (0b111)") {
        REQUIRE(alu(c, AND, 0b1100, 0b1010) == 0b1000);
        REQUIRE(alu(c, AND, 0x12345678, 0x0000FFFF) == 0x5678);
        REQUIRE(alu(c, AND, 0xF0F0F0F0, 0x0F0F0F0F) == 0);
    }
}

TEST_CASE("R-Type register semantics", "[cpu][execute][rtype]") {
    CPU c(MEM_SIZE);
    auto& r = CpuProbe::regs(c);

    SECTION("Writes to x0 are discarded, reads yield zero") {
        r.write(1, 42);
        exec(c, 0x00208033); // add x0, x1, x2
        REQUIRE(r.read(0) == 0);
        exec(c, 0x000085B3); // add x11, x1, x0
        REQUIRE(r.read(11) == 42);
    }

    SECTION("Operand aliasing") {
        r.write(1, 5);
        r.write(2, 3);
        exec(c, 0x002080B3); // add x1, x1, x2  (rd == rs1)
        REQUIRE(r.read(1) == 8);
        REQUIRE(r.read(2) == 3);

        r.write(3, 7);
        exec(c, 0x003181B3); // add x3, x3, x3  (all three alias)
        REQUIRE(r.read(3) == 14);

        r.write(1, 0x12345678);
        exec(c, 0x401082B3); // sub x5, x1, x1
        REQUIRE(r.read(5) == 0);
    }

    SECTION("Only rd is modified") {
        for (uint32_t i = 1; i < 32; ++i)
            r.write(i, 0xDEADBEEF);
        r.write(1, 5);
        r.write(2, 3);
        exec(c, ADD);
        REQUIRE(r.read(5) == 8);
        for (uint32_t i = 3; i < 32; ++i)
            if (i != 5)
                REQUIRE(r.read(i) == 0xDEADBEEF);
        REQUIRE(CpuProbe::result(c) == ExecResult::Continue);
    }
}

TEST_CASE("I-Type OP-IMM", "[cpu][execute][itype]") {
    CPU c(MEM_SIZE);

    SECTION("ADDI (0b000)") {
        REQUIRE(alui(c, 0x00508293, 3) == 8);          // addi x5, x1, 5
        REQUIRE(alui(c, 0xFFF08293, 1) == 0);          // addi x5, x1, -1
        REQUIRE(alui(c, 0x7FF08293, 0) == 2047);       // max positive imm
        REQUIRE(alui(c, 0x80008293, 0) == 0xFFFFF800); // min negative imm
        REQUIRE(alui(c, 0x00108293, 0xFFFFFFFF) == 0); // wraps, no trap
    }

    SECTION("SLLI (0b001)") {
        REQUIRE(alui(c, 0x00409293, 1) == 16);         // slli x5, x1, 4
        REQUIRE(alui(c, 0x01F09293, 1) == 0x80000000); // shamt 31
        REQUIRE(alui(c, 0x00009293, 0x12345678) == 0x12345678); // shamt 0
        REQUIRE(alui(c, 0x00409293, 0xF0000000) == 0); // bits fall off top
    }

    SECTION("SLTI (0b010) is signed") {
        REQUIRE(alui(c, 0x0050A293, 3) == 1);          // slti x5, x1, 5
        REQUIRE(alui(c, 0x0050A293, 5) == 0);          // equal is not less
        REQUIRE(alui(c, 0xFFF0A293, 0xFFFFFFFE) == 1); // -2 < -1
        REQUIRE(alui(c, 0xFFF0A293, 0) == 0);          //  0 < -1 is false
        REQUIRE(alui(c, 0x0000A293, 0x80000000) == 1); // INT_MIN < 0
    }

    SECTION("SLTIU (0b011) sign-extends, then compares unsigned") {
        REQUIRE(alui(c, 0x0050B293, 3) == 1); // sltiu x5, x1, 5
        REQUIRE(alui(c, 0x0010B293, 0) == 1); // seqz idiom
        REQUIRE(alui(c, 0x0010B293, 5) == 0);
        // imm -1 becomes 0xFFFFFFFF, the unsigned maximum: not a negative
        // bound.
        REQUIRE(alui(c, 0xFFF0B293, 0) == 1);
        REQUIRE(alui(c, 0xFFF0B293, 0xFFFFFFFF) == 0);
    }

    SECTION("XORI (0b100)") {
        REQUIRE(alui(c, 0x0F00C293, 0x0FF) == 0x00F); // xori x5, x1, 0xF0
        REQUIRE(alui(c, 0xFFF0C293, 0x12345678) == 0xEDCBA987); // not x5, x1
    }

    SECTION("SRLI_SRAI (0b101)") {
        REQUIRE(alui(c, 0x0040D293, 0x80000000) ==
                0x08000000);                           // srli, zero-fill
        REQUIRE(alui(c, 0x01F0D293, 0xFFFFFFFF) == 1); // srli shamt 31
        REQUIRE(alui(c, 0x4040D293, 0x80000000) ==
                0xF8000000); // srai, sign-fill
        REQUIRE(alui(c, 0x41F0D293, 0xFFFFFFFF) ==
                0xFFFFFFFF); // -1 >> 31 == -1
        REQUIRE(alui(c, 0x4040D293, 0x7FFFFFFF) ==
                0x07FFFFFF); // srai == srli here
        // imm[10] discriminates; it must not leak into the shift amount.
        REQUIRE(alui(c, 0x0040D293, 16) == 1);
        REQUIRE(alui(c, 0x4040D293, 16) == 1);
    }

    SECTION("ORI (0b110)") {
        REQUIRE(alui(c, 0x0F00E293, 0x00F) == 0x0FF);  // ori x5, x1, 0xF0
        REQUIRE(alui(c, 0xFFF0E293, 0) == 0xFFFFFFFF); // imm sign-extends
    }

    SECTION("ANDI (0b111)") {
        REQUIRE(alui(c, 0x0FF0F293, 0x12345678) == 0x78); // andi x5, x1, 0xFF
        REQUIRE(alui(c, 0xFFF0F293, 0x12345678) ==
                0x12345678); // mask is all ones
        REQUIRE(alui(c, 0xFF00F293, 0x12345678) ==
                0x12345670); // andi x5, x1, -16
    }
}

TEST_CASE("I-Type register semantics", "[cpu][execute][itype]") {
    CPU c(MEM_SIZE);
    auto& r = CpuProbe::regs(c);

    SECTION("Writes to x0 are discarded, reads yield zero") {
        r.write(1, 5);
        exec(c, 0x00508013); // addi x0, x1, 5
        REQUIRE(r.read(0) == 0);
        exec(c, 0x02A00293); // addi x5, x0, 42  (li idiom)
        REQUIRE(r.read(5) == 42);
    }

    SECTION("rd aliases rs1") {
        r.write(1, 7);
        exec(c, 0x00108093); // addi x1, x1, 1
        REQUIRE(r.read(1) == 8);
    }
}
