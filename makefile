CC=gcc
AS=nasm
IMG_SECTOS = 131072
IMG = ForgeOS.img
VM = bochs

DIR_SRC = ./src
DIR_BUILD = ./build
DIR_INC	= $(DIR_SRC)/include
DIR_LIB = $(DIR_SRC)/lib
DIR_KERNEL = $(DIR_SRC)/kernel

DIR_SCRIPTS = ./scripts

OBJ = $(DIR_BUILD)/kernel.o $(DIR_BUILD)/print.a.o $(DIR_BUILD)/print.c.o
BIN = $(DIR_BUILD)/boot.bin $(DIR_BUILD)/loader.bin $(DIR_BUILD)/kernel.bin

.PHONY: all run clean

all: $(DIR_BUILD) $(BIN) $(IMG)

# build dir
$(DIR_BUILD):
	mkdir $(DIR_BUILD)
# 编译boot
$(DIR_BUILD)/boot.bin:$(DIR_SRC)/boot/boot.asm $(DIR_INC)/boot.inc
	$(AS) -I $(DIR_INC) -o $@ $<

# 编译loader.asm
$(DIR_BUILD)/loader.bin :$(DIR_SRC)/boot/loader.asm $(DIR_INC)/boot.inc
	$(AS) -I $(DIR_INC) -o $@ $<
$(DIR_BUILD)/kernel.bin : $(OBJ) 

# 编译print
$(DIR_BUILD)/print.a.o: $(DIR_LIB)/print.asm $(DIR_INC)/boot.inc
	$(AS) -f elf32 -o $@ $<

# 编译print.c
$(DIR_BUILD)/print.c.o:$(DIR_LIB)/print.c $(DIR_INC)/print.h
	$(CC) -I $(DIR_INC) -m32 -c -o $@ $<
# 编译kernel.o
$(DIR_BUILD)/kernel.o : $(DIR_KERNEL)/kernel.c $(DIR_INC)/*
	$(CC) -I $(DIR_INC) -m32 -c -o $@ $<
# kernel.bin
$(DIR_BUILD)/kernel.bin : $(OBJ)
	ld -N -Ttext 0x100000 -m elf_i386 -e kernelMain -o $(DIR_BUILD)/kernel.elf32 $^
	objcopy -O binary -S -j .text -j .rodata -j .data $(DIR_BUILD)/kernel.elf32 $@

# 写操作系统镜像
$(IMG): $(BIN)
	# 准备空白的镜像
	dd if=/dev/zero of=$(IMG) bs=512 count=$(IMG_SECTOS)
	# 写boot
	dd if=$(DIR_BUILD)/boot.bin of=$(IMG) bs=512 count=1 conv=notrunc
	# loader
	dd if=$(DIR_BUILD)/loader.bin of=$(IMG) bs=512 count=2 seek=1 conv=notrunc
	# kernel
	dd if=$(DIR_BUILD)/kernel.bin of=$(IMG) bs=512 count=2 seek=3 conv=notrunc
run: $(IMG)
	$(VM) -q -f $(DIR_SCRIPTS)/bochsrc
clean:
	rm -fr $(DIR_BUILD)
	rm -f $(IMG)
	rm -f *.log
	rm -f *.ini
	rm -f *.lock