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
uint16 getCursor(void);
void setCursor(uint16 cursor);
uint8 setColor(uint8 color);
void cPutChar(uint8 color, uint8 asciiCh);
void putChar(uint8 asciiCh);
/*支持二进制、8进制、10进制、16进制*/
void putNum(uint32 num, uint32 base);
void uint2HexStr(char* buff, uint32 num, uint32 width);
// 清屏
void clsScreen(void);
//prink留在之后待用
void printk(const char* str);
void printkf(const char* fmt, ...);
#endif
