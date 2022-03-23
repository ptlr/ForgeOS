#ifndef LIB_IO_H
#define LIB_IO_H\
#include "stdint.h"
/* 向端口写入一个字节 */
static inline void outb(uint16 port, uint8 data){
    /*
     * %b0配合a表示al,%w1配合Nd表示使用dx,并且port是一个0~255范围内的数
     */
    asm volatile("outb %b0, %w1"::"a"(data),"Nd"(port));
}
/* 从端口读取一个字节 */
static inline uint8 inb(uint16 port){
    uint8 data;
    asm volatile("inb %w1,%b0":"=a"(data):"Nd"(port));
    return data;
}
#endif