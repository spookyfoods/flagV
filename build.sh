#!/usr/bin/env bash
set -euo pipefail

PREFIX=riscv64-unknown-elf
MEM_BASE=0x00000000

SRC="${1:?usage: ./build.sh file.s}"
STEM="${SRC%.s}"

# Assemble and Link using gcc driver to cleanly enforce 32-bit flat memory at 0x0
"$PREFIX-gcc" -march=rv32i -mabi=ilp32 -nostdlib -Wl,-Ttext="$MEM_BASE" "$SRC" -o "$STEM.elf"
"$PREFIX-objcopy" -O binary "$STEM.elf" "$STEM.bin"

echo "built $STEM.bin ($(stat -c%s "$STEM.bin") bytes)"
echo
echo "disassembly:"
"$PREFIX-objdump" -d -M numeric,no-aliases "$STEM.elf"
