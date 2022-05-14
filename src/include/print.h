#ifndef KERNEL_LIB_PRINT_H
#define KERNEL_LIB_PRINT_H
#include "stdint.h"
#define COLOR_BLINK 0x40
#define COLOR_HIGH_LIGHT 0x08

#define COLOR_BG_RED 0x40
#define COLOR_BG_GREEN 0x20
#define COLOR_BG_DARK 0x00

#define COLOR_BG_BLUE 0x10
#define COLOR_FG_RED 0x04
#define COLOR_FG_GREEN 0x02
#define COLOR_FG_BLUE 0x01
#define COLOR_FG_DRAK 0x00

#define DEFAULT_COLOR  COLOR_FG_GREEN | COLOR_FG_BLUE

uint8 setColor(uint8 color);
void cPutChar(uint8 color, uint8 asciiCh);
void putStr(const char * str);
void putHex(uint32 hexNum);
#endif
