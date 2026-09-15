# arraysum.s — fills memory with 0..99, reads it back, sums it, prints 4950.
#
# Deliberately does this in two passes so that both sw and lw are exercised.
# No .data section: everything lives in .text and the array is written at
# runtime, which keeps the linker script trivial.
#
# Register plan:
#   t0  loop counter i
#   t1  address pointer into the array
#   t2  limit (100)
#   t3  running sum
#   t4  value loaded back
#   a0  syscall argument
#   a7  syscall number
#
# Expected: Result: 4950   /   911 instructions, 100 loads, 100 stores

    .section .text
    .globl _start

_start:

# ---- pass 1: store 0..99 into memory ----------------------------------------

    lui  t1, 0x01     # t1 = 0x80001000, the array base.
                         # lui puts its immediate in bits 31:12, so 0x80001
                         # becomes 0x80001000. That is 4 KB past MEM_BASE,
                         # comfortably clear of this program (~100 bytes).
    li   t0, 0           # i = 0
    li   t2, 100         # limit = 100

fill_loop:
    sw   t0, 0(t1)       # memory[t1] = i
    addi t1, t1, 4       # advance one word
    addi t0, t0, 1       # i++
    blt  t0, t2, fill_loop   # signed "if i < 100, loop again"

# ---- pass 2: load them back and accumulate ----------------------------------

    lui  t1, 0x01     # reset the pointer to the array base
    li   t0, 0           # i = 0
    li   t3, 0           # sum = 0

sum_loop:
    lw   t4, 0(t1)       # t4 = memory[t1]
    add  t3, t3, t4      # sum += t4
    addi t1, t1, 4
    addi t0, t0, 1
    blt  t0, t2, sum_loop

# ---- print and halt ---------------------------------------------------------

    mv   a0, t3          # a0 = sum          (pseudo-op -> addi a0, t3, 0)
    li   a7, 1
    ecall

    li   a7, 93
    ecall
