#!/bin/bash

SRCFILE='custom/test.c'
OUT='custom/out.s'
DRIVER='custom/driver.c'

echo $SRCFILE
echo $OUT
echo $DRIVER


make
./bin/c_compiler -S $SRCFILE -o $OUT 
riscv64-unknown-elf-gcc -march=rv32imfd -mabi=ilp32d -o "custom/out" -c "custom/out.s"
echo 'compiled'
riscv64-unknown-elf-gcc -march=rv32imfd -mabi=ilp32d -static -o "custom/complete" "custom/out" "custom/driver.c"
spike pk "custom/complete"
riscv64-unknown-elf-objdump -d custom/complete > custom/disass.dump
