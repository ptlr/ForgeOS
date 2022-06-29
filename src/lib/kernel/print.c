#include "print.h"
#include "string.h"
#include "debug.h"
#include "stdio.h"
#include "interrupt.h"
#include "io.h"
uint8 displayColor = COLOR_FG_GREEN | COLOR_FG_BLUE;
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
void putStr(const char * str){
    int index = 0;
    while (str[index] != '\0')
    {
        //intrDisable();
        cPutChar(displayColor, str[index]);
        //intrEnable();
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