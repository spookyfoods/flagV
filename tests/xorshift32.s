# showcase.s  --  flagV RV32I demo
# No subroutines, no stack, no M extension (no mul/div), no jalr.
# Only loops, branches, load/store and ecall (a7=1 print int, a7=93 exit).
#
# Expected output, one value per line:
#   25 1060                                  (Part 1: prime count and sum, <= 100)
#   -250 -7 -1 0 3 15 15 34 88 912           (Part 2: sorted array)
#   8369910                                  (Part 3: 12345 * 678)
#   111 9232                                 (Part 4: Collatz(27) steps and peak)

    .option norelax          # stop ld rewriting la/li into gp-relative forms (gp is 0 here)
    .section .text
    .globl _start

    .equ SYS_PRINT_INT, 1
    .equ SYS_EXIT,      93
    .equ N,             100
    .equ SIEVE_BASE,    0x10000   # scratch RAM for the sieve (1 byte per number)
    .equ ARRAY_BASE,    0x20000   # scratch RAM for the array being sorted

_start:
# ============ Part 1: Sieve of Eratosthenes up to N ============
# mark[k] == 0 means "k is still a prime candidate", 1 means "crossed out".
# RAM starts zeroed, so every number starts as a candidate.
    li   s0, SIEVE_BASE
    li   s1, N
    li   t0, 2               # i
    li   t1, 4               # i*i, kept up to date without multiplying
sieve_outer:
    bltu s1, t1, sieve_count # stop once i*i > N
    add  t2, s0, t0
    lbu  t3, 0(t2)
    bnez t3, sieve_next      # i already crossed out: its multiples are too
    mv   t4, t1              # j starts at i*i
sieve_mark:
    bltu s1, t4, sieve_next  # j > N: done with this i
    add  t2, s0, t4
    li   t5, 1
    sb   t5, 0(t2)           # mark[j] = 1
    add  t4, t4, t0          # j += i
    j    sieve_mark
sieve_next:
    slli t2, t0, 1           # (i+1)^2 = i^2 + 2i + 1
    add  t1, t1, t2
    addi t1, t1, 1
    addi t0, t0, 1
    j    sieve_outer

sieve_count:
    li   t0, 2               # k
    li   a1, 0               # count
    li   a2, 0               # sum
count_loop:
    bltu s1, t0, count_done
    add  t2, s0, t0
    lbu  t3, 0(t2)
    bnez t3, count_skip
    addi a1, a1, 1
    add  a2, a2, t0
count_skip:
    addi t0, t0, 1
    j    count_loop
count_done:
    li   a7, SYS_PRINT_INT
    mv   a0, a1
    ecall                    # 25
    mv   a0, a2
    ecall                    # 1060

# ============ Part 2: copy array into RAM, insertion sort (signed) ============
    la   s2, input_data      # first word = length, then the elements
    li   s3, ARRAY_BASE
    lw   s4, 0(s2)           # s4 = length
    addi s2, s2, 4
    li   t0, 0
copy_loop:
    bge  t0, s4, copy_done
    slli t1, t0, 2           # offset = index * 4
    add  t2, s2, t1
    lw   t3, 0(t2)
    add  t2, s3, t1
    sw   t3, 0(t2)
    addi t0, t0, 1
    j    copy_loop
copy_done:

    li   t0, 1               # i
sort_outer:
    bge  t0, s4, sort_done
    slli t1, t0, 2
    add  t1, s3, t1
    lw   t2, 0(t1)           # key = a[i]
    addi t3, t0, -1          # j = i - 1
sort_inner:
    bltz t3, sort_insert     # ran off the front
    slli t4, t3, 2
    add  t4, s3, t4
    lw   t5, 0(t4)           # a[j]
    bge  t2, t5, sort_insert # key >= a[j]: found the spot
    sw   t5, 4(t4)           # shift a[j] right into a[j+1]
    addi t3, t3, -1
    j    sort_inner
sort_insert:
    addi t4, t3, 1
    slli t4, t4, 2
    add  t4, s3, t4
    sw   t2, 0(t4)           # a[j+1] = key
    addi t0, t0, 1
    j    sort_outer
sort_done:

    li   t0, 0
print_loop:
    bge  t0, s4, print_done
    slli t1, t0, 2
    add  t1, s3, t1
    lw   a0, 0(t1)
    ecall                    # a7 is still SYS_PRINT_INT
    addi t0, t0, 1
    j    print_loop
print_done:

# ============ Part 3: multiply with shifts and adds ============
# For each 1 bit in the multiplier, add the (shifted) multiplicand.
    li   t0, 12345           # multiplicand
    li   t1, 678             # multiplier
    li   a0, 0               # product
mul_loop:
    beqz t1, mul_done
    andi t2, t1, 1
    beqz t2, mul_skip
    add  a0, a0, t0
mul_skip:
    slli t0, t0, 1
    srli t1, t1, 1
    j    mul_loop
mul_done:
    ecall                    # 8369910

# ============ Part 4: Collatz sequence from 27 ============
# even: n = n/2   odd: n = 3n + 1   (3n = (n << 1) + n)
    li   t0, 27              # n
    li   t1, 0               # steps
    mv   t2, t0              # peak
    li   t3, 1
collatz_loop:
    beq  t0, t3, collatz_done
    andi t4, t0, 1
    beqz t4, collatz_even
    slli t5, t0, 1
    add  t0, t5, t0
    addi t0, t0, 1
    j    collatz_step
collatz_even:
    srli t0, t0, 1
collatz_step:
    addi t1, t1, 1
    bgeu t2, t0, collatz_loop
    mv   t2, t0              # new peak
    j    collatz_loop
collatz_done:
    mv   a0, t1
    ecall                    # 111
    mv   a0, t2
    ecall                    # 9232

    li   a0, 0
    li   a7, SYS_EXIT
    ecall

# ============ Data (placed after the code, never executed) ============
    .align 2
input_data:
    .word 10
    .word 34, -7, 0, 912, -250, 15, 15, 3, -1, 88
