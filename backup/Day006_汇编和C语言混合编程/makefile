CC=gcc
AS=nasm
IMG_SECTOS = 131072
IMG = ForgeOS.img
VM = bochs
INC= -I ./
.PHONY: run clean

all:
	# 编译boot.asm
	$(AS) $(INC) -o boot.bin boot.asm
	# 编译loader.asm
	$(AS) $(INC) -o loader.bin loader.asm
	# 编译print
	$(AS) -f elf32 -o print.o print.asm
	# 编译kernel
	$(CC) -I . -m32 -c -o kernel.o kernel.c
	# 链接
	ld -N -Ttext 0 -m elf_i386 -e kernelMain -o kernel.elf32 kernel.o print.o
	# bin
	#objcopy -R .got.plt -R .note -R .comment kernel.elf32 kernel.bin
	objcopy -O binary -S -j .text -j .rodata -j .data kernel.elf32 kernel.bin
	# 准备空白的镜像
	dd if=/dev/zero of=$(IMG) bs=512 count=$(IMG_SECTOS)
	# 写boot
	dd if=boot.bin of=$(IMG) bs=512 count=1 conv=notrunc
	# loader
	dd if=loader.bin of=$(IMG) bs=512 count=2 seek=1 conv=notrunc
	# kernel
	dd if=kernel.bin of=$(IMG) bs=512 count=2 seek=3 conv=notrunc
run:
	$(VM) -q -f bochsrc
clean:
	rm -f *.o
	rm -f *.bin
	rm -f *.elf32
	rm -f $(IMG)
	rm -f *.log
	rm -f *.ini
	rm -f *.lock