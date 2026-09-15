    .section .text
    .globl _start

_start:
    li   a0, 42          # a0 = value to print   (pseudo-op -> addi a0, x0, 42)
    li   a7, 1           # a7 = 1  -> "print a0 as signed decimal"
    ecall

    li   a7, 93          # a7 = 93 -> "halt"
    ecall
