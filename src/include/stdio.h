#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#include "stdint.h"

typedef char* va_list;
#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL

uint32 vsprintf(char* str, const char* format, va_list ap);
uint32 printf(const char* format, ...);
#endif