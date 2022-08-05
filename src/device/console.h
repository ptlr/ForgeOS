#ifndef DEVICE_CONSOLE_H
#define DEVICE_CONSOLE_H
/*初始化终端*/
void consoleInit(void);
/*通过终端打印字符串*/
void consolePrint(const char* msg);
#endif