#ifndef LIB_IO_H
#define LIB_IO_H
#include "stdint.h"
/* 向端口写入一个字节 */
static inline void outb(uint16 port, uint8 data){
    /*
     * %b0配合a表示al,%w1配合Nd表示使用dx,并且port是一个0~255范围内的数
     */
    asm volatile("outb %b0, %w1"::"a"(data),"Nd"(port));
}
// 将addr处wordCount个字节写到port端口
static inline void outsw(uint16 port, void* addr, uint32 wordCount){
    asm volatile("cld; rep outsw" : "+S" (addr), "+c" (wordCount): "d" (port):"memory");
}
/* 从端口读取一个字节 */
static inline uint8 inb(uint16 port){
    uint8 data;
    asm volatile("inb %w1,%b0":"=a"(data):"Nd"(port));
    return data;
}
/* 将从端口port处读取wordCount个字写入addr处 */
static inline void insw(uint16 port, void* addr, uint32 wordCount){
    asm volatile("cld; rep insw" : "+D" (addr), "+c" (wordCount): "d" (port):"memory");
}
#endif