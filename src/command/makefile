CC=gcc
CFLAGS = -c -m32  -nostdlib -W -fno-builtin -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers
INC= -I ../lib
BIN=prog-no-arg
LD_FLAGS = -m elf_i386 -e main -Ttext 0x80000000 -o $(BIN) prog-no-arg.o
LIB_OBJ=  string.c.o syscall.c.o stdio.c.o assert.c.o format.c.o number.c.o
CRT=$(addprefix ../../build/, $(LIB_OBJ))
all:
	$(CC) $(CFLAGS) -o prog-no-arg.o prog-no-arg.c
	$(LD) $(LD_FLAGS) $(CRT)
clean:
	rm -f *.o
	rm -f $(BIN)