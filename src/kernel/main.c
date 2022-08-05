#include "init.h"
#include "debug.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
#include "string.h"
#include "stdio.h"
#include "memory.h"
#include "thread.h"
#include "console.h"

char* MSG_KERNEL = "[05] kernel start\n";
void func(void* funcArg);
void kernelMain(void)
{
    printf("\n\n\n\n%s", MSG_KERNEL);
    init();
    //intrDisable();
    //strTest();
    //stdioTest();
    //debugTest();
    //memTest();
    startThread("KERNEL_THREAD_A", 31, func, "ARG-A ");
    startThread("KERNEL_THREAD_B", 8, func, "ARG-B ");
    intrEnable();
    while(1){
        consolePrint("main ");
    }
    while(1);
}
void putNumTest(void){
    for(uint32 num = 0; num < 17; num++){
        putStr("BIN: ");putNum(num,2);putStr(", DEC: ");putNum(num,10);putStr(", HEX: ");putNum(num,16);putChar('\n');
    }
}
void memTest(void){
    void* p1 = allocKernelPages(1);
    void* p2 = allocKernelPages(1);
    void* p3 = allocKernelPages(3);

    printf("P1 vaddr = 0x%x\nP2 vaddr = 0x%x\nP3 vaddr = 0x%x\n",(uint32)p1, (uint32)p2, (uint32)p3);
}
void stdioTest(void){
   // 十六进制测试
   printf("Hex number: %x\n",16);
   // 十进制测试
   printf("Dec number: %d\n",16);
   // 字符串测试
   printf("Str : %s\n%s\n%c\n","Hello World!", "Forge OS!",'F');
   printf("Char test: %c%c%c\n",'A','S','D');
}

void strTest(void)
{
    char* strA = "ABC\n";
    char* strB = "DEF\n";
    char* result = "ABC\nDEF\n";
    char* AAndB = strcat(strA, strB);
    putStr(AAndB);
    putStr(result);
    ASSERT(strcmp(AAndB, result) == 0);
}

// 线程中调用的函数
void func(void* funArg){
    char* para = funArg;
    while (1)
    {
        /*intrDisable();
        putStr(para);
        intrEnable();*/
        consolePrint(para);
    }
}