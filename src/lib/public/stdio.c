#include "stdio.h"
#include "print.h"
#include "../include/stdint.h"
#include "debug.h"
#include "string.h"
#include "interrupt.h"
#include "syscall.h"
/* 整型转换成字符（Integer to ASCII）
 *
 */
static void itoa(uint32 value, char** buffAddrPtr, uint8 base)
{
    uint32 m = value % base;
    uint32 i = value / base;
    if(i){ // 倍数不为零，继续递归调用
        itoa(i, buffAddrPtr, base);
    }
    if(m < 10){ // 如果余数是0～9，转换成字符'0'~'9'
        *((*buffAddrPtr)++) = m + '0';
    }else{ // 如果余数是10～15，转换成字符'A'~'F'
        *((*buffAddrPtr)++) = m - 10 + 'A';
    }
}
/*
 * 调用这个函数前需要初始化参数ap,否则会出现bug
 */
uint32 vsprintf(char* str, const char* format, va_list ap)
{
    char* bufPtr = str;
    const char* indexPtr = format;
    char indexChar = *indexPtr;
    int32 argInt;
    char* argStr = "";
    while (indexChar)
    {
        if(indexChar != '%'){
            *(bufPtr++) = indexChar;
            indexChar = *(++indexPtr);
            continue;
        }
        indexChar = *(++indexPtr);
        
        switch (indexChar)
        {
        case 'd':
            argInt = va_arg(ap, int);
            if(argInt < 0){
                argInt = 0 - argInt;
                *bufPtr++ = '-'; 
            }
            itoa(argInt, &bufPtr, 10);
            indexChar = *(++indexPtr);
            break;
        case 'x':
            argInt = va_arg(ap, int);
            itoa(argInt, &bufPtr, 16);
            indexChar = *(++indexPtr);
        break;
        case 'c':
            *(bufPtr++) = va_arg(ap, char);
            indexChar = *(++indexPtr);
            break;
        case 's':
            argStr = va_arg(ap, char*);
            strcat(bufPtr, argStr);
            bufPtr += strlen(argStr);
            indexChar = *(++indexPtr);
        default:
            break;
        }
    }
    return strlen(str);
}
/*函数实现方式不安全，不建议使用*/
uint32 printf(const char* format, ...)
{
    va_list args;
    /**
     * 根据调用约定，从右往左压入参数。
     * format的地址+4等于可变参数的第一个参数。
     * va_start把args地址指向format,va_args把ap+4后返回
     */
    va_start(args, format);
    char buff[1024] = {0};
    vsprintf(buff, format, args);
    va_end(args);
    return write(buff);
}