CC=gcc
AS=nasm
IMG = ForgeOS.img
VM = bochs
.PHONY: run clean

all:
	# 编译boot.asm
	$(AS) -o boot.bin boot.asm
	# 编译loader.asm
	$(AS) -o loader.bin loader.asm
	# 准备空白的镜像
	dd if=/dev/zero of=$(IMG) bs=512 count=2880
	# 写boot
	dd if=boot.bin of=$(IMG) bs=512 count=1 conv=notrunc
	# loader
	dd if=loader.bin of=$(IMG) bs=512 count=1 seek=1 conv=notrunc
run:
	$(VM) -q -f bochsrc
clean:
	rm -f *.bin
	rm -f $(IMG)
	rm -f *.log