#include "print.h"
#include "string.h"
#include "debug.h"
#include "stdio.h"
uint8 displayColor = COLOR_FG_GREEN | COLOR_FG_BLUE;
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
        cPutChar(displayColor, str[index]);
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