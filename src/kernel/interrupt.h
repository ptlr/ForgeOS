#ifndef INTERRUPT_H
#define INTERRUPT_H
#include "io.h"
#include "stdint.h"
#include "printk.h"
#include "constant.h"

#define PIC_M_CTRL   0x20  // 主片控制端口
#define PIC_M_DATA   0x21  // 主片数据端口
#define PIC_S_CTRL   0xA0  // 从片控制端口
#define PIC_S_DATA   0xA1  // 从片数据端口
// 中断号
#define INTR_0x20_TIMEER   0x20
#define INTR_0x21_KEYBOARD 0x21  
// void* 是指向32地址类型
typedef void* intrHandler;
#define IDT_DESC_CNT    0x81

// 系统调用
extern uint32 syscallHandler(void);

/*中断门描述符结构体*/
struct GateDesc
{
   uint16 funcOffsetLowWord;
   uint16 selector;
   uint8 dcount;        // 置0
   uint8 attribute;
   uint16 funcOffsetHighWord; 
};
void initIdt(int (* step)(void));

// 中断相关

/* 定义中断状态：
 * INTR_OFF = 0, 关中断
 * INTR_ON  = 1, 开中断
 */
enum IntrStatus
{
   INTR_OFF,
   INTR_ON
};
// eflags IF位掩码
#define EFLAGS_IF_MASK 0x00000200
// 获取EFLAGS
uint32 getEflags(void);
// 开中断
enum IntrStatus intrEnable(void);
// 关中断
enum IntrStatus intrDisable(void);
// 获取中断状态
enum IntrStatus getIntrStatus(void);
// 设置中断状态
enum IntrStatus setIntrStatus(enum IntrStatus);
// 注册中断
void registerHandler(uint8 intrVecNum, intrHandler func);
#endif