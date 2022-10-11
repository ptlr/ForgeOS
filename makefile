CC=gcc
# C flags
CFLAGS	 = -g -c -m32
CFLAGS	+= -nostdlib
CFLAGS	+= -fno-builtin
CFLAGS	+= -fno-stack-protector
CFLAGS	+= -Wall
# 去掉函数前面的下划线
#CFLAGS  += -fleading-underscore --no-leading-underscore
#CFLAGS	+= -o0
#CFLAGS	+= -Wstrict-prototypes
#CFLAGS	+= -Wmissing-prototypes

AS=nasm

# 汇编参数
ASFLAGS= -f elf32

IMG_SECTOS = 131072
IMG = ForgeOS.img
VM = bochs
DIR_SRC = ./src
DIR_INC = ./src/include/
DIR_BUILD = ./build/

INC_DIRS = include/ lib/public/ lib/kernel/ lib/user kernel/ thread/ \
			device/ user/

INC = $(addprefix -I ./src/, $(INC_DIRS))

DIR_LIB = $(DIR_SRC)/lib

DIR_PUBLIC_LIB = $(DIR_LIB)/public/
DIR_KERNEL_LIB = $(DIR_LIB)/kernel/
DIR_USER_LIB   = $(DIR_LIB)/user/

DIR_KERNEL = $(DIR_SRC)/kernel
DIR_THREAD = $(DIR_SRC)/thread
DIR_DEVICE = $(DIR_SRC)/device
DIR_USER   = $(DIR_SRC)/user

LOADER_SECTOR_CNT 	= 2
# expr 后面的表达式运算符和操作数之间需要空格
KERNEL_SEEK			= $(shell expr $(LOADER_SECTOR_CNT) + 1)
# 增加到64KiB，共128个扇区
KERNEL_SECTOR_CNT	= 128
DIR_SCRIPTS = ./scripts

OBJ_NAMES = kernel.o printk.a.o interrupt.a.o printk.c.o main.o interrupt.c.o\
		init.c.o timer.c.o debug.c.o string.c.o stdio.c.o bitmap.c.o memory.c.o \
		thread.c.o list.c.o switch2.a.o sync.c.o console.c.o keyboard.c.o \
		ioqueue.c.o tss.c.o process.c.o syscall.c.o syscall-init.c.o

BIN_NAMES = boot.bin loader.bin kernel.bin

OBJ = $(addprefix $(DIR_BUILD), $(OBJ_NAMES))
BIN = $(addprefix $(DIR_BUILD), $(BIN_NAMES))

.PHONY: all run clean

all: $(DIR_BUILD) $(BIN) $(IMG)
	
# build dir
$(DIR_BUILD):
	mkdir -p $(DIR_BUILD)

$(DIR_BUILD)/kernel.bin : $(OBJ) 

# 编译boot
$(DIR_BUILD)boot.bin:$(DIR_SRC)/boot/boot.asm $(DIR_INC)/constant.inc
	$(AS) -I $(DIR_INC) -o $@ $<

# 编译loader.asm
$(DIR_BUILD)loader.bin :$(DIR_SRC)/boot/loader.asm $(DIR_INC)/constant.inc
	$(AS) -I $(DIR_INC) -o $@ $<

# 编译kernel.o
$(DIR_BUILD)kernel.o : $(DIR_KERNEL)/kernel.asm $(DIR_INC)/constant.inc
	$(AS) $(ASFLAGS) -I $(DIR_INC) -o $@ $<

# 编译print
$(DIR_BUILD)printk.a.o: $(DIR_KERNEL_LIB)/printk.asm $(DIR_INC)/constant.inc
	$(AS) $(ASFLAGS) -o $@ $<
# 编译interrupt.a.o
$(DIR_BUILD)interrupt.a.o:$(DIR_KERNEL)/interrupt.asm $(DIR_INC)/*
	$(AS) $(ASFLAGS) -o $@ $<
# 编译switch2.a.o
$(DIR_BUILD)switch2.a.o:$(DIR_THREAD)/switch2.asm $(DIR_INC)/*
	$(AS) $(ASFLAGS) -o $@ $<
# 编译printk.c
$(DIR_BUILD)printk.c.o:$(DIR_KERNEL_LIB)/printk.c $(DIR_KERNEL_LIB)/printk.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译interrupt.c
$(DIR_BUILD)interrupt.c.o:$(DIR_KERNEL)/interrupt.c $(DIR_INC)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
$(DIR_BUILD)init.c.o:$(DIR_KERNEL)/init.c $(DIR_INC)/* $(DIR_KERNEL)/init.h
	$(CC) $(CFLAGS) $(INC) -I $(DIR_KERNEL_LIB) -o $@ $<
$(DIR_BUILD)timer.c.o:$(DIR_KERNEL)/timer.c $(DIR_INC)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
$(DIR_BUILD)debug.c.o : $(DIR_KERNEL)/debug.c $(DIR_INC)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译string库
$(DIR_BUILD)string.c.o:$(DIR_PUBLIC_LIB)/string.c $(DIR_INC)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译标准输入输出库
$(DIR_BUILD)stdio.c.o:$(DIR_PUBLIC_LIB)/stdio.c $(DIR_INC)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译bitmap
$(DIR_BUILD)bitmap.c.o:$(DIR_KERNEL_LIB)bitmap.c $(DIR_KERNEL_LIB)bitmap.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
$(DIR_BUILD)memory.c.o: $(DIR_KERNEL)/memory.c $(DIR_KERNEL_LIB)/printk.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译thread
$(DIR_BUILD)thread.c.o:$(DIR_THREAD)/thread.c $(DIR_KERNEL_LIB)/*
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译list
$(DIR_BUILD)list.c.o:$(DIR_KERNEL_LIB)/list.c $(DIR_KERNEL_LIB)/list.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译sync
$(DIR_BUILD)sync.c.o:$(DIR_THREAD)/sync.c $(DIR_THREAD)/sync.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译console
$(DIR_BUILD)console.c.o:$(DIR_DEVICE)/console.c $(DIR_DEVICE)/console.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译keybord
$(DIR_BUILD)keyboard.c.o:$(DIR_DEVICE)/keyboard.c $(DIR_DEVICE)/keyboard.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译ioqueue
$(DIR_BUILD)ioqueue.c.o:$(DIR_DEVICE)/ioqueue.c $(DIR_DEVICE)/ioqueue.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 编译TSS
$(DIR_BUILD)tss.c.o: $(DIR_USER)/tss.c $(DIR_USER)/tss.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# 用户进程
$(DIR_BUILD)process.c.o: $(DIR_USER)/process.c $(DIR_USER)/process.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# syscall
$(DIR_BUILD)syscall.c.o : $(DIR_USER_LIB)/syscall.c $(DIR_USER_LIB)/syscall.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# syscall-init
$(DIR_BUILD)syscall-init.c.o : $(DIR_USER)/syscall-init.c $(DIR_USER)/syscall-init.h
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# kernel main
$(DIR_BUILD)main.o: $(DIR_KERNEL)/main.c $(DIR_INC)/* $(DIR_KERNEL_LIB)/printk.h	
	$(CC) $(CFLAGS) $(INC) -o $@ $<
# kernel.bin
$(DIR_BUILD)kernel.bin : $(OBJ)
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
debug: $(IMG)
	$(DVM) -q -f $(DIR_SCRIPTS)/bochsrc
clean:
	rm -fr $(DIR_BUILD)
	rm -f $(IMG)
	rm -f *.log
	rm -f *.ini
	rm -f *.lock