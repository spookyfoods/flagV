#include "system.h"
#include "csr.h"
#include <bitset>
#include <cstdlib>
#include <print>

Memory::Memory(size_t size, Counters& _ctrs) : m_data(size, 0), c_ctrs(_ctrs) {}

void Memory::fail_loudly(uint32_t address, uint32_t pc, uint32_t width,
                         const char* reason) const {
    std::println(
        stderr,
        "[MEMORY FAULT] {} | Addr: 0x{:x} | PC: 0x{:x} | Width: {} bytes",
        reason, address, pc, width);
    std::abort();
}

size_t Memory::translate(uint32_t address, uint32_t pc, uint32_t width) const {
    if (address < MEM_BASE) {
        fail_loudly(address, pc, width, "Address Below MEM_BASE");
    }
    size_t offset = address - MEM_BASE;
    if (offset + width > m_data.size()) {
        fail_loudly(address, pc, width, "Address out of upper bounds");
    }
    return offset;
}

uint32_t Memory::fetch32(uint32_t address) {
    size_t offset = translate(address, 0, 4);
    return static_cast<uint32_t>(m_data[offset]) |
           (static_cast<uint32_t>(m_data[offset + 1]) << 8) |
           (static_cast<uint32_t>(m_data[offset + 2]) << 16) |
           (static_cast<uint32_t>(m_data[offset + 3]) << 24);
}

uint8_t Memory::read8(uint32_t address, uint32_t pc) {
    size_t offset = translate(address, pc, 1);
    c_ctrs.inc_loads();
    return m_data[offset];
}

uint16_t Memory::read16(uint32_t address, uint32_t pc) {
    size_t offset = translate(address, pc, 2);
    c_ctrs.inc_loads();
    return static_cast<uint16_t>(
        static_cast<uint32_t>(m_data[offset]) |
        (static_cast<uint32_t>(m_data[offset + 1]) << 8));
}

uint32_t Memory::read32(uint32_t address, uint32_t pc) {
    size_t offset = translate(address, pc, 4);
    c_ctrs.inc_loads();
    return static_cast<uint32_t>(m_data[offset]) |
           (static_cast<uint32_t>(m_data[offset + 1]) << 8) |
           (static_cast<uint32_t>(m_data[offset + 2]) << 16) |
           (static_cast<uint32_t>(m_data[offset + 3]) << 24);
}

CPU::CPU(uint32_t _memSize)
    : m_ctrs(), m_regs(), m_mem(_memSize, m_ctrs), m_pc(Memory::MEM_BASE),
      m_result(ExecResult::Continue), m_priv_lvl(PrivLvl::M) {

    // Setting the value of register 2 aka sp (stack pointer)
    // Since the stack grows downwards, from a higher address to a lower address
    m_regs.write(2, Memory::MEM_BASE + _memSize);
}

void CPU::step() {
    uint32_t pcInc = 4;
    try {
        auto word = fetch(m_pc);
        Instruction ins = decode(word);

        m_result = execute_context(ins, pcInc);
    } catch (const Trap& t) {
        take_trap_exception(t);
        m_result = ExecResult::Continue;
        return;
    }
    if (m_result == ExecResult::Fault) {
        return;
    }

    m_pc = m_pc + pcInc;
    m_ctrs.inc_icount();
    if (m_tohost_addr) {
        const uint32_t value = m_mem.fetch32(m_tohost_addr);
        if (value != 0) {
            m_tohost_val = value;
            m_result = ExecResult::Halt;
        }
    }
}

ExecResult CPU::run(uint64_t max_steps) {
    for (auto s = 0ull; m_result == ExecResult::Continue; ++s) {
        if (s >= max_steps) {
            std::println(stderr, "flagV: step limit {} reached", max_steps);
            dump_state(*this);
            return ExecResult::Fault;
        }
        step();
    }

    if (m_result == ExecResult::Fault) {
        std::println(stderr, "ExecResult is FAULT!");
        dump_state(*this);
    }

    return m_result;
}

uint32_t CPU::fetch(uint32_t addr) { return m_mem.fetch32(addr); }

uint32_t CPU::reg(uint8_t i) const { return m_regs.read(i); }

void CPU::load(std::vector<uint32_t> insVec, uint32_t startAddress) {
    m_pc = startAddress;
    for (auto i = 0u; i < insVec.size(); i++) {
        uint32_t val = insVec[i];
        size_t offset = m_mem.translate(startAddress + 4 * i, m_pc, 4);
        for (auto j = 0zu; j < 4; j++) {
            m_mem.m_data[offset + j] = static_cast<uint8_t>(0x000000FF & val);
            val >>= 8;
        }
    }
}

void CPU::take_trap_exception(const Trap& t) noexcept {

    m_csrfile.write(CSR::MEPC, m_pc);
    m_csrfile.write(CSR::MCAUSE, t.mcause);
    m_csrfile.write(CSR::MTVAL, t.mtval);

    //  Read-Modify-Write

    /*Read*/
    std::bitset<32> old_mstatus = m_csrfile.read(CSR::MSTATUS);

    /*Modify*/
    // mstatus.MPIE <- mstatus.MIE
    old_mstatus.set(7, old_mstatus[3]);
    // mstatus.MIE <- 0
    old_mstatus.set(3, 0);

    // mstatus.MPP <- previousMode
    // Currently only setting U(=00)
    old_mstatus.set(11, 0);
    old_mstatus.set(12, 0);

    /*Write*/

    m_csrfile.write(CSR::MSTATUS, static_cast<reg_t>(old_mstatus.to_ulong()));

    // set hart's privilege level to Machine Mode
    m_priv_lvl = PrivLvl::M;

    // pc <- mtvec.BASE
    // Limiting to only DIRECT mode
    m_pc = m_csrfile.read(MTVEC) &
           static_cast<reg_t>(static_cast<int32_t>(~0) << 2);

    // Write back modified mstatus
}

template <> void Memory::write<8>(uint32_t address, uint32_t pc, uint32_t val) {
    size_t offset = translate(address, pc, 1);
    c_ctrs.inc_stores();
    for (auto i = 0zu; i < 1; i++) {
        m_data[offset + i] = static_cast<uint8_t>(0x000000FF & val);
        val >>= 8;
    }
}

template <>
void Memory::write<16>(uint32_t address, uint32_t pc, uint32_t val) {
    size_t offset = translate(address, pc, 2);
    c_ctrs.inc_stores();
    for (auto i = 0zu; i < 2; i++) {
        m_data[offset + i] = static_cast<uint8_t>(0x000000FF & val);
        val >>= 8;
    }
}

template <>
void Memory::write<32>(uint32_t address, uint32_t pc, uint32_t val) {
    size_t offset = translate(address, pc, 4);
    c_ctrs.inc_stores();
    for (auto i = 0zu; i < 4; i++) {
        m_data[offset + i] = static_cast<uint8_t>(0x000000FF & val);
        val >>= 8;
    }
}

void dump_state(const CPU& cpu) {

    // x0  zero    x8  s0/fp   x16 a6     x24 s8
    // x1  ra      x9  s1      x17 a7     x25 s9
    // x2  sp      x10 a0      x18 s2     x26 s10
    // x3  gp      x11 a1      x19 s3     x27 s11
    // x4  tp      x12 a2      x20 s4     x28 t3
    // x5  t0      x13 a3      x21 s5     x29 t4
    // x6  t1      x14 a4      x22 s6     x30 t5
    // x7  t2      x15 a5      x23 s7     x31 t6

    static constexpr std::array<const char*, 32> ABI_NAMES = {
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

    std::println(stderr,
                 "{:<7}= 0x{:08x}  |  icount={:<5} loads={:<5} stores={:<5}",
                 "pc", cpu.m_pc, cpu.m_ctrs.get_icount(),
                 cpu.m_ctrs.get_loads(), cpu.m_ctrs.get_stores());

    for (uint8_t i = 1; i < 32; i++) {
        std::println(stderr, "x{:<2}/{:<4} 0x{:08x}", i, ABI_NAMES[i],
                     cpu.m_regs.read(i));
    }
    std::print(stderr, "\n\n\n");
}
