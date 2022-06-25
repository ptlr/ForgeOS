#include "init.h"
#include "debug.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
#include "string.h"
#include "stdio.h"
#include "memory.h"

char* MSG_KERNEL = "[05] kernel start\n";
void strTest(void);
void stdioTest(void);
void debugTest(void);
void memTest(void);
void kernelMain(void);
void kernelMain(void)
{
    printf("\n\n\n\n%s", MSG_KERNEL);
    init();
    //intrEnable();
    //intrDisable();
    //strTest();
    //stdioTest();
    //debugTest();
    memTest();
    while (1);
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

void debugTest(void)
{
    logInfor("This is an normal log");
    logWaring("This is an Waring log");
    logError("This is an Error log");
}