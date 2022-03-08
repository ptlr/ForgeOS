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
	# 编译kernel
	$(AS) -o kernel.bin kernel.asm

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
	rm -f *.bin
	rm -f $(IMG)
	rm -f *.log
	rm -f *.ini
	rm -f *.lock