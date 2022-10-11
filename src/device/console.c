#include "console.h"
#include "sync.h"
#include "printk.h"
#include "interrupt.h"
#include "stdarg.h"
#include "stdio.h"

// 控制台锁
static struct Lock consoleLock;

/*初始化终端*/
void consoleInit(){
    printk("[11] init console\n");
    lockInit(&consoleLock, "ConsoleLock");
}
/*获取终端*/
void consoleAcquire(){
    lockAcquire(&consoleLock);
}
/*释放终端*/
void consoleRelease(){
    lockRelease(&consoleLock);
}

void consolePrint(const char* msg){
    consoleAcquire();
    printk(msg);
    consoleRelease();
}
/*通过终端打印数字*/
void consoleNum(uint32 num, int base){
    consoleAcquire();
    putNum(num, base);
    consoleRelease();
}
void consolePrintf(const char* format, ...){
    va_list args;
    va_start(args, format);
    char buffer[1024] = {0};
    vsprintf(buffer, format, args);
    va_end(args);
    consoleAcquire();
    printk(buffer);
    consoleRelease();
}