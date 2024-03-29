#!/bin/bash -x

BASE_FB=${1:-0x8F800000}

GCCDIR=~dolbeau/LITEX/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14
GCCPFX=riscv64-unknown-elf-
GCCLINK=${GCCDIR}/bin/${GCCPFX}gcc

#GCCDIR=/opt/rv32bk
#GCCPFX=riscv32-buildroot-linux-gnu-

GCCDIR=~dolbeau2/LITEX/buildroot-rv32/output/host
GCCPFX=riscv32-buildroot-linux-gnu-

GCC=${GCCDIR}/bin/${GCCPFX}gcc
OBJCOPY=${GCCDIR}/bin/${GCCPFX}objcopy

## for compiler that support Zbt, not yet mainline GCC
#OPT="-O3 -DCOMPILER_SUPPORT_FSR" #-fno-inline
#ARCH=rv32im_zba_zbb_zbt
## for mainline GCC recent enough to have Zba/Zbb support (older unsupported)
OPT="-O3" #-fno-inline
ARCH=rv32im_zba_zbb

PARAM="-DBASE_FB=${BASE_FB} -DGOBLIN_NUBUS"

if test "x$1" != "xASM"; then
	$GCC $OPT -S -o blit_goblin.s $PARAM -march=$ARCH -mabi=ilp32 -mstrict-align -fno-builtin-memset -nostdlib -ffreestanding -nostartfiles blit_goblin.c
fi
$GCC     $OPT -c -o blit_goblin.o $PARAM -march=$ARCH -mabi=ilp32 -mstrict-align -fno-builtin-memset -nostdlib -ffreestanding -nostartfiles blit_goblin.s &&
$GCCLINK $OPT    -o blit_goblin   $PARAM -march=$ARCH -mabi=ilp32 -T blit_goblin_nubus.lds  -nostartfiles blit_goblin.o &&
$OBJCOPY  -O binary -j .text -j .rodata blit_goblin blit_goblin_nubus.raw
