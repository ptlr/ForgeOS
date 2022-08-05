#ifndef __STDARG_H
#define __STDARG_H
#include "stdint.h"
typedef char* va_list;
#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL
#endif