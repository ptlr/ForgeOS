#include "print.h"
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

void putHex(uint32 hexNum){
    char* hexStr = "0x________";
    for (uint32 index = 9; index > 1; index--)
    {
        int8 mHexNum = hexNum & 0x0000000F;
        if(mHexNum < 10){
            hexStr[index] = mHexNum + '0';
        }else{
            hexStr[index] = mHexNum - 10 + 'A';
        }
        hexNum = hexNum >> 4;
    }
    putStr(hexStr);
}
