#include "printk.h"
#include "string.h"
#include "debug.h"
#include "stdio.h"
#include "interrupt.h"
#include "io.h"
#include "constant.h"
#include "stdarg.h"
#include "stdio.h"
#include "format.h"

uint8 displayColor = COLOR_FG_GREEN | COLOR_FG_BLUE;
uint16 getCursor(void){
    uint16 cursor = 0;
    outb(0x03D4, 0x0E);
    cursor = inb(0x03D5);
    cursor = cursor << 8;
    outb(0x03D4, 0x0F);
    cursor += inb(0x03D5);
    return cursor;
}
void setCursor(uint16 cursor){
    // 设置高8位
    outb(0x03D4,0x0E);
    outb(0x03D5, (uint8)((cursor & 0xFF00) >> 8));
    // 设置低8位
    outb(0x03D4,0x0F);
    outb(0x03D5, (uint8)(cursor & 0x00FF));
}
uint8 setColor(uint8 color)
{ 
    uint8 oldColor = displayColor;
    displayColor = color;
    return oldColor;
}
void putChar(uint8 asciiCh){
    cPutChar(displayColor, asciiCh);
}
// ToDo: 有缺陷，不能处理负数
void putNum(uint32 num, uint32 base){
    char buffer[32];
    uint32 result = num;
    for(int index = 31; index >= 0; index--){
        uint32 remainder = result % base;
        result = result / base;
        if(remainder < 10){
            buffer[index] = remainder + '0';
        }else{
            buffer[index] = remainder + 'A' - 10;
        }
    }
    /* 这里出现一个奇怪的问题：gcc编译器开启优化的时候，isValid会被置1
     * 
     */
    uint32 index = 0;
    // 告诉编译器不要优化这个变量
    volatile bool isValid = false;
    while (index < 32)
    {
        if(!isValid){
            if(buffer[index] != '0' || index == 31) isValid = true;
        }
        // 不能使用分支，否则会缺少一位不输出
        if(isValid) putChar(buffer[index]);
        index++;
    }
    
}

void uint2HexStr(char* buff, uint32 num, uint32 width)
{
    memset(buff, '0', width);
    uint32 buffIndex = width - 1;
    for(int32 index = 0; index < 8; index++)
    {
        int8 mByte = (num >> (index * 4)) & 0x000F;
        if(mByte < 10){
            buff[buffIndex] = mByte + '0';
        }else{
            buff[buffIndex] = mByte - 10 + 'A';
        }
        buffIndex--;
    }
}
// 清屏
void clsScreen(){
    
}
void printk(const char* str){
    int index = 0;
    while (str[index] != '\0')
    {
        cPutChar(displayColor, (char)str[index]);
        index++;
    }   
}

void printkf(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    char buffer[1024] = {0};
    format(buffer, fmt, args);
    va_end(args);
    printk(buffer);
}