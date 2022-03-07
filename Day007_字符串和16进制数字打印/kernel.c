#include "print.h"
#include "stdint.h"

char* MSG_KERNEL = "[05] kernel start\n";
char* MSG_TEST = "[06] TEST MSG\n";
void kernelMain(void)
{
    putStr("\n\n\n\n");
    putStr(MSG_KERNEL);
    putStr(MSG_TEST);
    putHex(0x12345678);
    cPutChar(0x07,'\n');
    putHex(0xABCDEF);
    cPutChar(0x07,'\n');
    putHex(10);
    cPutChar(0x07,'\n');
    putHex(0);
    while (1);
}
