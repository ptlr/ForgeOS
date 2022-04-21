#include "init.h"
#include "timer.h"
void init()
{
    putStr("[06] start init\n");
    initIdt();
    initTimer();
}