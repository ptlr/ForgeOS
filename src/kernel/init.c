#include "init.h"
#include "timer.h"
#include "memory.h"
void init(void)
{
    putStr("[06] start init\n");
    initIdt();
    initTimer();
    initMem();
}