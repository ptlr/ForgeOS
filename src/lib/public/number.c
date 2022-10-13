#include "number.h"

/* 整型转换成字符（Integer to ASCII）
 *
 */
void itoa(uint32 value, char** buffAddrPtr, uint8 base)
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