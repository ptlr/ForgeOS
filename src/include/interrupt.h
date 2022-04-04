#ifndef INTERRUPT_H
#define INTERRUPT_H
#include "io.h"
#include "stdint.h"
#include "print.h"
#include "constant.h"

#define PIC_M_CTRL   0x20  // 主片控制端口
#define PIC_M_DATA   0x21  // 主片数据端口
#define PIC_S_CTRL   0xA0  // 从片控制端口
#define PIC_S_DATA   0xA1  // 从片数据端口
// void* 是指向32地址类型
typedef void* intrHandler;
#define IDT_DESC_CNT    0x30
/*中断门描述符结构体*/
struct GateDesc
{
   uint16 funcOffsetLowWord;
   uint16 selector;
   uint8 dcount;        // 置0
   uint8 attribute;
   uint16 funcOffsetHighWord; 
};
//static void makeIdtDesc(struct GateDesc* gateDesc, uint8 attr, intrHandler function);
//static void initIdtDesc();
//static void initPic();
void initIdt();
#endif