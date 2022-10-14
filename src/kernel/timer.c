#include "timer.h"
#include "io.h"
#include "printk.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"
#include "stdio.h"
// 系统启动以来的滴答数
uint32 ticks = 0;
static void timerIntrHandler(void){
    struct TaskStruct* currentThread = runningThread();
    // 检测栈溢出
    ASSERT(currentThread->stackMagic == 0x19940520);
    currentThread->elapsedTicks++;
    ticks++;
    //printf("SCH-O\n");
    if (currentThread->ticks == 0)
    {   // 如果时间片用完，调用新的线程
        //printf("SCH-B1\n");
        schedule();
    }else{
        //printf("SCH-B2\n");
        currentThread->ticks--;
    }
}
static void setFrequency(uint8 counterPort, uint8 counterNo, uint8 rwl, uint8 mode, uint16 countValue)
{
    outb(CONTROL_PORT, (uint8)(counterNo << 6 | rwl << 4 | mode << 1 | BCD_FALSE));
    outb(counterPort, (uint8)countValue);
    outb(counterPort, (uint8)countValue >> 8);
}

void initTimer(int step)
{
    printkf("[%02d] init timer\n", step);
    uint16 countVal = FREQUENCY_MAX / FREQUENCY_24Hz;
    setFrequency(PORT_COUNTER0, SC_COUNTER0, RWL_LOW_HIGH, COUNTOR_MODE2, countVal);
    // 注册时钟中断处理函数
    //printf("TIH: 0x%x\n", timerIntrHandler);
    registerHandler(INTR_0x20_TIMEER, timerIntrHandler);
    //while(1);
}