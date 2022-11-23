#include "console.h"
#include "sync.h"
#include "printk.h"
#include "interrupt.h"
#include "stdarg.h"
#include "stdio.h"
#include "format.h"

// 控制台锁
static struct Lock consoleLock;

/*初始化终端*/
void consoleInit(int (* step)(void)){
    printkf("[%02d] init console\n", step());
    lockInit(&consoleLock, "ConsoleLock");
}
/*获取终端*/
void consoleAcquire(void){
    lockAcquire(&consoleLock);
}
/*释放终端*/
void consoleRelease(void){
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
void consolePrintf(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    char buffer[1024] = {0};
    format(buffer, fmt, args);
    va_end(args);
    consoleAcquire();
    printk(buffer);
    consoleRelease();
}