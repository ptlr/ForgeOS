#ifndef LIB_PUBLIC_FORMAT_H
#define LIB_PUBLIC_FORMAT_H
#include "stdarg.h"
#include "stdint.h"
uint32 format(char* str, const char* fmt, va_list ap);
//void padPrint(char* buffer, int32 buffLen, void* ptr, char format);
#endif