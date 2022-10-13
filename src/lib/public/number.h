#ifndef LIB_PUBLIC_NUMBER_H
#define LIB_PUBLIC_NUMBER_H
#include "stdio.h"
void itoa(uint32 value, char** buffAddrPtr, uint8 base);
void itoaw(uint32 value, char** buffPtr, uint8 base,uint32 width);
#endif