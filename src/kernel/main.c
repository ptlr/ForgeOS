#include "init.h"
#include "debug.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
#include "string.h"
#include "stdio.h"

char* MSG_KERNEL = "[05] kernel start\n";
//char* MSG_KERNEL = "HW";
void test(void);
void strTest(void);
void stdioTest(void);
void debugTest(void);
void kernelMain(void);
void kernelMain(void)
{
    putStr("\n\n\n\n");
    putStr(MSG_KERNEL);
    //test();
    init();
    //ASSERT(1 == 2);
    intrDisable();
    strTest();
    stdioTest();
    debugTest();
    while (1);
}
void test(void)
{
    for (uint32 index = 0; index < 0xFFFFFFFF; index++)
    {
        putStr("LINE : ");putHex(index);cPutChar(0x07, '\n');
    }
    
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