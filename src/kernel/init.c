#include "init.h"
#include "timer.h"
void init(void)
{
    putStr("[06] start init\n");
    initIdt();
    initTimer();
}