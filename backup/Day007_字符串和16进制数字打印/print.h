#ifndef KERNEL_LIB_PRINT_H
#define KERNEL_LIB_PRINT_H
#include "stdint.h"
void cPutChar(uint8 color, uint8 asciiCh);
void putStr(char * str);
void putHex(uint32 hexNum);
#endif
