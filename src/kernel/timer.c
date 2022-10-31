#include "timer.h"
#include "io.h"
#include "printk.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"
#include "stdio.h"
// 时钟中断频率
#define IRQ0_FREQUENCY          100
// 中断周期，单位毫秒
#define MS_DURATION_PER_INTR    (1000 / IRQ0_FREQUENCY)
// 系统中断开启以来的滴答数
uint32 ticks = 0;
// 以tick为单位的sleep, 任何以时间为单位的睡眠都会转成ticks的形式
static void tickSleep(uint32 sleepTicks){
    uint32 startTick = ticks;
    uint32 durationTick = 0;
    while(ticks -  startTick < sleepTicks){
        threadYeild();
    }
    durationTick = ticks - startTick;
    printkf("ST = %8d, CT: %8d, DT: %8d, Time: %8dms\n", startTick, ticks, durationTick, durationTick * 1000);
}
// 以毫秒秒为时间单位的睡眠
void msSleep(uint32 msecs){
    uint32 sleepTicks = DIV_ROUND_UP(msecs, MS_DURATION_PER_INTR);
    tickSleep(sleepTicks);
}
static void timerIntrHandler(void){
    struct TaskStruct* currentThread = runningThread();
    // 检测栈溢出
    ASSERT(currentThread->stackMagic == 0x19940520);
    currentThread->elapsedTicks++;
    ticks++;
    //printkf("CURRENT_TICK: %16d\n", ticks);
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

void initTimer(int (* step)(void))
{
    printkf("[%02d] init timer\n", step());
    uint16 countVal = FREQUENCY_MAX / IRQ0_FREQUENCY;
    setFrequency(PORT_COUNTER0, SC_COUNTER0, RWL_LOW_HIGH, COUNTOR_MODE2, countVal);
    // 注册时钟中断处理函数
    //printf("TIH: 0x%x\n", timerIntrHandler);
    registerHandler(INTR_0x20_TIMEER, timerIntrHandler);
    //while(1);
}