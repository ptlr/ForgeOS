#ifndef KERNEL_CONSTANT_H
#define KERNEL_CONSTANT_H
#include "stdint.h"
// 内核页目录表开始虚拟地址
#define KERNEL_PAGE_DIR_TABLE_VADDR 0xC0200000
// GDT描述符属性
#define DESC_G_4K           1
#define DESC_D_32           1
#define DESC_L              0   // 64位代码标记，32位系统，此处置0
#define DESC_AVL            0   // CPU不使用，暂置0
#define DESC_P              1
#define DESC_DPL_0          0
#define DESC_DPL_1          1
#define DESC_DPL_2          2
#define DESC_DPL_3          3
/* 代码段和数据段属于存储段，TSS和各种门描述符属于系统段
 * S位为1时表示存储段，为0时表示系统段
 */
#define DESC_S_CODE         1
#define DESC_S_DATA         1
#define DESC_S_SYS          0
// X=1, C=0, R=0, A=0: 代码段，可执行、非依从、不可读、已访问a清0
#define DESC_S_TYPE_CODE    8
// X=0, E=0, R=0, A=0: 数据，不可执行、向上扩展、可写、已访问a清0
#define DESC_S_TYPE_DATA    2
// B位置0，表示任务不忙
#define DESC_S_TYPE_TSS     9
// 请求特权级
#define RPL0                    0
#define RPL1                    1
#define RPL2                    2
#define RPL3                    3

#define TI_GDT                  0
#define TI_IDT                  1

#define SELECTOR_KERNEL_CODE    ((1 << 3) + (TI_GDT << 2) + RPL0) 
#define SELECTOR_KERNEL_DATA    ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_KERNEL_STACK   SELECTOR_KERNEL_DATA                // 与内核数据段相同
#define SELECTOR_KERNEL_TEXT    ((3 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_TSS            ((4 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_USER_CODE      ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_USER_DATA      ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_USER_STACK     SELECTOR_USER_DATA                  // 与用户数据段相同

// IDT属性
#define IDT_DESC_P              1
#define IDT_DESC_DPL0           0
#define IDT_DESC_DPL3           3
#define IDT_DESC_TYPE_32B      0xE // 32位的门

#define IDT_DESC_ATTR_DPL0      ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_TYPE_32B)
#define IDT_DESC_ATTR_DPL3      ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_TYPE_32B)

#define NULL ((void*)0)

#define bool int
#define true 1
#define false 0

#define PAGE_SIZE 4096
/* GDT属性 */
#define GDT_ATTR_HIGH   ((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))
#define GDT_CODE_ATTR_LOW_DPL3  ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_S_TYPE_CODE)
#define GDT_DATA_ATTR_LOW_DPL3  ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + DESC_S_TYPE_DATA)
/* TSS描述符属性 */
#define TSS_DESC_D  0
#define TSS_ATTR_HIGH   (DESC_G_4K << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x0
#define TSS_ATTR_LOW    (DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_S_TYPE_TSS

/* EFLAGS属性 */
#define EFLAGS_MBS  (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
#define EFLAGS_IF_0 (0 << 9)
// 用于测试用户程序在非系统调用下的IO
#define EFLAGS_IOPL_3   (3 << 12)
#define EFLAGS_IOPL_0   (0 << 12)
/* GDT描述符*/
struct GdtDesc{
    uint16 limitLowWord;
    uint16 baseLowWord;
    uint8 baseMidByte;
    uint8 attrLowByte;
    uint8 LimiHighAttrHigh;
    uint8 baseHighByte;
};
// 编程技巧：这里传入的VALUE和STEP肯能会是一个表达式，使用小括号可以保证优先级
#define DIV_ROUND_UP(VALUE, STEP) ((VALUE + STEP - 1) / (STEP))
#endif