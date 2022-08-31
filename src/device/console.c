#include "console.h"
#include "sync.h"
#include "print.h"
#include "interrupt.h"
// 控制台锁
static struct Lock consoleLock;

/*初始化终端*/
void consoleInit(){
    putStr("[11] init console\n");
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
    putStr(msg);
    consoleRelease();
}
/*通过终端打印数字*/
void consoleNum(uint32 num, int base){
    consoleAcquire();
    putNum(num, base);
    consoleRelease();
}