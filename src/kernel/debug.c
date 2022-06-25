#include "debug.h"
#include "print.h"
#include "interrupt.h"
#include "stdio.h"
void panicSpin(char* fileName, int line, const char* func, const char* condition)
{
    // 条件不不满足，关中断
    intrDisable();
    printf("ERROR:\nFILE: %s\nLINE:%d\nCONDITION: %s\n", fileName, line, condition);
    //putStr("ERROR:\n");
    //putStr("FILE: ");putStr(fileName);putStr("\n");
    //putStr("LINE: ");putHex(line);putStr("\n");
    //putStr("CONDITION: ");putStr(condition);putStr("\n");
    while (1);
}
void log(const char* label, const char* msg, uint8 color)
{ 
    uint8 oldColor = setColor(color);
    printf("%s:: %s\n", label, msg);
    setColor(oldColor);
}
void logInfor(const char* msg){
    log("Infor ",msg, DEFAULT_COLOR);
}
void logWaring( const char* msg)
{
    log("Waring",msg,COLOR_FG_GREEN | COLOR_FG_RED | COLOR_HIGH_LIGHT);
}
void logError( const char* msg)
{
    log("Error ",msg, COLOR_FG_RED);
}