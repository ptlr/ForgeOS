#ifndef LIB_STDIO_H
#define LIB_STDIO_H
#include "stdarg.h"
uint32 vsprintf(char* str, const char* format, va_list ap);
uint32 printf(const char* format, ...);
#endif