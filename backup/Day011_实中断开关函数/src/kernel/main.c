#include "init.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
char* MSG_KERNEL = "[05] kernel start\n";
//char* MSG_KERNEL = "HW";
void test();
void kernelMain(void)
{
    putStr("\n\n\n\n");
    putStr(MSG_KERNEL);
    //test();
    init();
    //asm volatile("sti"); // 开启中断
    intrEnable();
    while (1);
}
void test()
{
    for (uint32 index = 0; index < 0xFFFFFFFF; index++)
    {
        putStr("LINE : ");putHex(index);cPutChar(0x07, '\n');
    }
    
}
