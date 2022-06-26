#ifndef KERNEL_CONSTANT_H
#define KERNEL_CONSTANT_H
// 请求特权级
#define RPL0                    0
#define RPL1                    1
#define RPL2                    2
#define RPL3                    3

#define TI_GDT                  0
#define TI_IDT                  1

#define SELECTOR_KERNEL_CODE    ((1 << 3) + (TI_GDT << 2) + RPL0) 
#define SELECTOR_KERNEL_DATA    ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_KERNEL_STACK   ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_KERNEL_TEXT    ((3 << 3) + (TI_GDT << 2) + RPL0)

// IDT属性
#define IDT_DESC_P              1
#define IDT_DESC_DPL0           0
#define IDT_DESC_DPL3           3
#define IDT_DESC_TYPE_32B      0xE // 32位的门

#define IDT_DESC_ATTR_DPL0      ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_TYPE_32B)
#define IDT_DESC_ATTR_DPL3      ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_TYPE_32B)

#define NULL ((void*)0)

#define bool int
#define true 1
#define false 0

#define PAGE_SIZE 4096
#endif