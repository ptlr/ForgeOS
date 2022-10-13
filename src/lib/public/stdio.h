#ifndef LIB_STDIO_H
#define LIB_STDIO_H
#include "stdarg.h"
// ToDo:: 测试后删除
void fmtTest(const char* format, ...);
uint32 vsprintf(char* str, const char* format, va_list ap);
uint32 printf(const char* format, ...);
#endif