#include "stdio.h"
#include "printk.h"
#include "../include/stdint.h"
#include "debug.h"
#include "string.h"
#include "interrupt.h"
#include "syscall.h"
#include "number.h"
#include "format.h"
/*函数实现方式不安全，不建议使用*/
uint32 printf(const char* fmt, ...)
{
    va_list args;
    /**
     * 根据调用约定，从右往左压入参数。
     * format的地址+4等于可变参数的第一个参数。
     * va_start把args地址指向format,va_args把ap+4后返回
     */
    va_start(args, fmt);
    char buff[1024] = {0};
    format(buff, fmt, args);
    va_end(args);
    return write(buff);
}