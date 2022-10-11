#ifndef DEVICE_CONSOLE_H
#define DEVICE_CONSOLE_H
#include "stdint.h"
/*初始化终端*/
void consoleInit(void);
/*通过终端打印字符串*/
void consolePrint(const char* msg);
/*通过中断打印数字*/
void consoleNum(uint32 num, int base);
void consolePrintf(const char* format, ...);
#endif