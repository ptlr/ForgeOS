#include "init.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
void init(void)
{
    putStr("[06] start init\n");
    initIdt(); // IDT需要最先初始化
    initTimer();
    initMem();
    initThreadEnv();
    consoleInit();
}