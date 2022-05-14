CC=gcc
# C flags
CFLAGS=-c -m32 -nostdlib -fno-builtin -fno-stack-protector -Wall -Wstrict-prototypes -Wmissing-prototypes

AS=nasm

# 汇编参数
ASFLAGS=

IMG_SECTOS = 131072
IMG = ForgeOS.img
VM = bochs

DIR_SRC = ./src
DIR_BUILD = ./build
DIR_INC	= $(DIR_SRC)/include
DIR_LIB = $(DIR_SRC)/lib
DIR_KERNEL = $(DIR_SRC)/kernel

LOADER_SECTOR_CNT 	= 2
# expr 后面的表达式运算符和操作数之间需要空格
KERNEL_SEEK			= $(shell expr $(LOADER_SECTOR_CNT) + 1)
# 增加到16KiB，共32个扇区
KERNEL_SECTOR_CNT	= 32
DIR_SCRIPTS = ./scripts

OBJ = 	$(DIR_BUILD)/kernel.o $(DIR_BUILD)/print.a.o $(DIR_BUILD)/interrupt.a.o\
		$(DIR_BUILD)/print.c.o $(DIR_BUILD)/main.o $(DIR_BUILD)/interrupt.c.o\
		$(DIR_BUILD)/init.c.o $(DIR_BUILD)/timer.c.o $(DIR_BUILD)/debug.c.o\
		$(DIR_BUILD)/string.c.o $(DIR_BUILD)/stdio.c.o

BIN = $(DIR_BUILD)/boot.bin $(DIR_BUILD)/loader.bin $(DIR_BUILD)/kernel.bin

.PHONY: all run clean

all: $(DIR_BUILD) $(BIN) $(IMG)

# build dir
$(DIR_BUILD):
	mkdir $(DIR_BUILD)

$(DIR_BUILD)/kernel.bin : $(OBJ) 

# 编译boot
$(DIR_BUILD)/boot.bin:$(DIR_SRC)/boot/boot.asm $(DIR_INC)/constant.inc
	$(AS) -I $(DIR_INC) -o $@ $<

# 编译loader.asm
$(DIR_BUILD)/loader.bin :$(DIR_SRC)/boot/loader.asm $(DIR_INC)/constant.inc
	$(AS) -I $(DIR_INC) -o $@ $<

# 编译kernel.o
$(DIR_BUILD)/kernel.o : $(DIR_KERNEL)/kernel.asm $(DIR_INC)/constant.inc
	$(AS) -I $(DIR_INC) -f elf32 -o $@ $<

# 编译print
$(DIR_BUILD)/print.a.o: $(DIR_LIB)/print.asm $(DIR_INC)/constant.inc
	$(AS) -f elf32 -o $@ $<
# 编译interrupt.a.o
$(DIR_BUILD)/interrupt.a.o:$(DIR_KERNEL)/interrupt.asm $(DIR_INC)/*
	$(AS) -f elf32 -o $@ $<

# 编译print.c
$(DIR_BUILD)/print.c.o:$(DIR_LIB)/print.c $(DIR_INC)/print.h
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
# 编译interrupt.c
$(DIR_BUILD)/interrupt.c.o:$(DIR_KERNEL)/interrupt.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
$(DIR_BUILD)/init.c.o:$(DIR_KERNEL)/init.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
$(DIR_BUILD)/timer.c.o:$(DIR_KERNEL)/timer.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
$(DIR_BUILD)/debug.c.o : $(DIR_KERNEL)/debug.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
# 编译string库
$(DIR_BUILD)/string.c.o:$(DIR_LIB)/string.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
# 编译标准输入输出库
$(DIR_BUILD)/stdio.c.o:$(DIR_LIB)/stdio.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
# kernel main
$(DIR_BUILD)/main.o: $(DIR_KERNEL)/main.c $(DIR_INC)/*	
	$(CC) -I $(DIR_INC) $(CFLAGS) -o $@ $<
# kernel.bin
$(DIR_BUILD)/kernel.bin : $(OBJ)
	ld -N -Ttext 0xC0100000 -m elf_i386 -e kernelStart -o $(DIR_BUILD)/kernel.elf32 $^
	objcopy -O binary -S -j .text -j .rodata -j .data $(DIR_BUILD)/kernel.elf32 $@

# 写操作系统镜像
$(IMG): $(BIN)
# 准备空白的镜像
	dd if=/dev/zero of=$(IMG) bs=512 count=$(IMG_SECTOS)
# 写boot
	dd if=$(DIR_BUILD)/boot.bin of=$(IMG) bs=512 count=1 conv=notrunc
# loader
	dd if=$(DIR_BUILD)/loader.bin of=$(IMG) bs=512 count=$(LOADER_SECTOR_CNT) seek=1 conv=notrunc
# kernel
	dd if=$(DIR_BUILD)/kernel.bin of=$(IMG) bs=512 count=$(KERNEL_SECTOR_CNT) seek=$(KERNEL_SEEK) conv=notrunc
run: $(IMG)
	$(VM) -q -f $(DIR_SCRIPTS)/bochsrc
clean:
	rm -fr $(DIR_BUILD)
	rm -f $(IMG)
	rm -f *.log
	rm -f *.ini
	rm -f *.lock