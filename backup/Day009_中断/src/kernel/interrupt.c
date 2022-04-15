#include "stdint.h"
#include "constant.h"
#include "interrupt.h"

static struct GateDesc IDT[IDT_DESC_CNT];
/* 这里的intrEntryTable是中断入口地址，中断对应的函数的地址被存放到了这里
 * 每项是32bit的数据
 */
extern intrHandler intrEntryTable[IDT_DESC_CNT];

static void makeIdtDesc(struct GateDesc* gateDesc, uint8 attr,intrHandler function)
{
    gateDesc->funcOffsetLowWord = (uint32)function & 0x0000FFFF;
    gateDesc->selector = SELECTOR_KERNEL_CODE;
    gateDesc->dcount = 0;
    gateDesc->attribute = attr;
    gateDesc->funcOffsetHighWord = ((uint32)function & 0xFFFF0000) >> 16;
}
static void initIdtDesc()
{
    int index;
    for (index = 0; index < IDT_DESC_CNT; index++)
    {
        makeIdtDesc(&IDT[index], IDT_DESC_ATTR_DPL0, intrEntryTable[index]);
    }
    
}
static void initPic()
{
    putStr("[08] init PIC\n");
    // 初始化主片
    outb(PIC_M_CTRL, 0x11); // ICW1：边缘触发，级联8259，需要ICW4
    outb(PIC_M_DATA, 0x20); // ICW2: 起始中断号为0x20，即IR0~IR7为中断0x20~0x27

    outb(PIC_M_DATA, 0x04); // ICW3: IR2接从片(0b0000_0100)
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式，正常EOI模式
    /* 初始化从片 */
    outb(PIC_S_CTRL, 0x11); // ICW1: 边缘触发，级联8259，需要ICW4
    outb(PIC_S_DATA, 0x28); // ICW2: 起始中断号为0x28，即IR0~IR8为中断0x28~0x2F

    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的IR2
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式，正常EOI模式

    /*打开主片上的IRO, 即，只产生时钟中断*/
    outb(PIC_M_DATA, 0xFE); // 主片OCW1: 开启IR0(时钟中断)
    outb(PIC_S_DATA, 0xFF); // 从片OCW1：屏蔽所有中断
}
void initIdt()
{
    putStr("[07] init IDT\n");cPutChar(0x07,'\n');
    initIdtDesc();
    initPic();
    //while (1);
    putHex((uint32)&IDT);cPutChar(0x07,'\n');
    //uint64 preAddr = ;
    /*加载IDT
     * 先把32位地址转换成uint32
     * 再把uint32转化成uint64
     */
    putStr("IDT SIZE: ");putHex(sizeof(IDT));cPutChar(0x07,'\n');
    uint64 idtOperand = ((uint64)((uint32)&IDT) << 16) | (sizeof(IDT) - 1);
    putStr("IDT ADDR L: ");putHex((uint32)idtOperand);cPutChar(0x07,'\n');
    putStr("IDT ADDR H: ");putHex((uint32)(idtOperand >> 32));cPutChar(0x07,'\n');
    asm volatile ("lidt %0"::"m"(idtOperand));
}
