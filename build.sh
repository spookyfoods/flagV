#!/usr/bin/env bash
# build.sh — assemble a .s into a flat .bin your emulator can load.
#
#   ./build.sh tests/arraysum.s   ->   tests/arraysum.bin
#
# -march=rv32im   : the ISA you implemented (base integer + mul/div)
# -mabi=ilp32     : 32-bit integers, longs and pointers
# -m elf32lriscv  : REQUIRED. Your toolchain defaults to 64-bit and ld will
#                   refuse to link 32-bit objects without it.
# -Ttext=...      : MUST equal MEM_BASE in your emulator. If these two
#                   numbers ever disagree, every branch target is wrong and
#                   it looks exactly like a decode bug.

set -euo pipefail

PREFIX=riscv64-unknown-elf
MEM_BASE=0x00000000

SRC="${1:?usage: ./build.sh file.s}"
STEM="${SRC%.s}"

"$PREFIX-as"      -march=rv32im -mabi=ilp32 "$SRC" -o "$STEM.o"
"$PREFIX-ld"      -m elf32lriscv -Ttext="$MEM_BASE" "$STEM.o" -o "$STEM.elf"
"$PREFIX-objcopy" -O binary "$STEM.elf" "$STEM.bin"

echo "built $STEM.bin ($(stat -c%s "$STEM.bin") bytes)"
echo
echo "disassembly:"
"$PREFIX-objdump" -d -M numeric,no-aliases "$STEM.elf"
