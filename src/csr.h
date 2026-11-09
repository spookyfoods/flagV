#ifndef CSR_H
#define CSR_H
#include <concepts>
#include <cstdint>
#include <utility>
enum class PrivLvl : uint8_t { U = 0b00, S = 0b01, M = 0b11 };
using reg_t = uint32_t;

enum CSR : uint16_t {
    MVENDORID = 0xF11,
    MARCHID = 0XF12,
    MIMPID = 0XF13,
    MHARTID = 0xF14,
    MSTATUS = 0x300,
    MISA = 0X301,
    MEDELEG = 0x302,
    MIDELEG = 0X303,
    MIE = 0X304,
    MTVEC = 0x305,
    MSCRATCH = 0X340,
    MEPC = 0X341,
    MCAUSE = 0x342,
    MTVAL = 0x343,
    MIP = 0x344,
};

struct Trap {
    reg_t mcause;
    reg_t mtval;
};

class CsrFile {
  private:
    reg_t mstatus = 0;

    reg_t mepc = 0;
    reg_t mcause = 0;
    reg_t mtval = 0;
    reg_t mscratch = 0;
    reg_t mtvec = 0;

  public:
    bool isAccessValid(uint16_t funct12, PrivLvl priv, bool willWrite);

    template <std::same_as<reg_t> T> void write(uint16_t funct12, T value) {
        switch (funct12) {
        case MEPC:
            mepc = value;
        case MCAUSE:
            mcause = value;
        case MTVAL:
            mtval = value;
        case MTVEC:
            mtvec = value;
        case MISA:
            [[fallthrough]];
        case MVENDORID:
            [[fallthrough]];
        case MARCHID:
            [[fallthrough]];
        case MIMPID:
            [[fallthrough]];
        case MHARTID:
            [[fallthrough]];
        case MSTATUS:
            [[fallthrough]];
        default:
            return;
            // std::unreachable();
        }
    }

    reg_t read(uint16_t funct12) const {

        switch (funct12) {
        case MEPC:
            return mepc;
        case MCAUSE:
            return mcause;
        case MTVAL:
            return mtval;
        case MTVEC:
            return mtvec;
        case MSTATUS:
            return mstatus;

        case MISA:
            [[fallthrough]];
        case MVENDORID:
            [[fallthrough]];
        case MARCHID:
            [[fallthrough]];
        case MIMPID:
            [[fallthrough]];
        case MHARTID:
            [[fallthrough]];
        default:
            return 0;
            // std::unreachable();
        }
    }
};

enum class Exception : uint32_t {
    IllegalInstruction = 2,
    Breakpoint = 3,
    EnvCallFromU = 8,

};

// clang-format off
/**
31      30 29                                        0
+---------+------------------------------------------+
|Interrupt|       Exception Code (WLRL)               |
+---------+------------------------------------------+
*/
/**
Exceptions:
| Code         | Exception                      | Typical source                                           |
|--------------+--------------------------------+----------------------------------------------------------|
| *0*            | *Instruction address misaligned* | branch / jump to a target not 4-aligned (without C)      |
| *1*            | *Instruction access fault*       | fetch from an address that doesn't exist / isn't allowed |
| *2*            | *Illegal instruction*            | bad encoding, CSR privilege violation, ~mret~ in U         |
| *3*            | *Breakpoint*                     | ~ebreak~ (or a debug trigger)                              |
| 4            | Load address misaligned        | misaligned load (if not supported)                       |
| *5*            | *Load access fault*              | load from a bad address                                  |
| 6            | Store/AMO address misaligned   | misaligned store                                         |
| *7*            | *Store/AMO access fault*         | store to a bad address                                   |
| *8*            | *Environment call from U-mode*   | ~ecall~ in U                                               |
| *9*            | *Environment call from S-mode*   | ~ecall~ in S                                               |
| 10           | /reserved/                       |                                                          |
| *11*           | *Environment call from M-mode*   | ~ecall~ in M                                               |
| 12           | Instruction page fault         | virtual memory (later)                                   |
| 13           | Load page fault                | virtual memory                                           |
| 14           | /reserved/                       |                                                          |
| 15           | Store/AMO page fault           | virtual memory                                           |
| 16           | Double trap                    | Smdbltrp extension                                       |
| 18           | Software check                 | control-flow-integrity extensions                        |
| 19           | Hardware error                 | corrupted data detected                                  |
| 24-31, 48-63 | /custom use/                     |                                                          |

Interrupts:
| Code | Interrupt                    | mcause on RV32 |
|------+------------------------------+----------------|
|    1 | Supervisor software interrupt | 0x80000001    |
|    3 | Machine software interrupt   | 0x80000003     |
|    5 | Supervisor timer interrupt   | 0x80000005     |
|    7 | Machine timer interrupt      | 0x80000007     |
|    9 | Supervisor external interrupt | 0x80000009    |
|   11 | Machine external interrupt   | 0x8000000B     |
|   13 | Counter-overflow interrupt   | 0x8000000D     |
| >=16 | platform use                 |                |


mtval:
| Trap                                             | If ~mtval~ is nonzero, it holds                                     |
|--------------------------------------------------+-------------------------------------------------------------------|
| breakpoint, misaligned, access fault, page fault | the *faulting address*                                              |
| ... misaligned access that faults partway        | the address of the part that faulted                              |
| instruction fault, variable-length instructions  | the address of the faulting part (~mepc~ still points at the start) |
| *illegal instruction*                              | the *instruction bits* (optional)                                   |
| software-check                                   | a code (0 none, 2 landing pad, 3 shadow stack)                    |
| everything else (e.g. ~ecall~, interrupts)         | zero                                                              |

 **/
// clang-format on

#endif // CSR_H
