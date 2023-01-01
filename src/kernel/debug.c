#include "debug.h"
#include "printk.h"
#include "interrupt.h"
#include "stdio.h"

void panicSpin(char* fileName, int line, const char* func, const char* condition)
{
    // 条件不不满足，关中断
    intrDisable();
    setColor(COLOR_BG_DARK | COLOR_FG_RED);
    printk("\nERROR:\n");
    printk("FILE: ");printk(fileName);printk("\n");
    printk("LINE: ");putNum(line, 10);printk("\n");
    printk("CONDITION: ");printk(condition);printk("\n");
    while (1);
}
static void log(const char* msg, uint8 color){
    enum IntrStatus oldStatus = intrDisable();
    uint8 oldColor = setColor(color);
    printk(msg);
    setColor(oldColor);
    setIntrStatus(oldStatus);
}
void logWarning(const char* msg){
    log(msg, COLOR_BG_DARK | COLOR_FG_GREEN | COLOR_FG_RED);
}
void logError(const char* msg){
    log(msg, COLOR_BG_DARK | COLOR_FG_RED);
}