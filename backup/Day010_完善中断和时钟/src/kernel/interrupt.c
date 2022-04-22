#include "stdint.h"
#include "constant.h"
#include "interrupt.h"

static struct GateDesc IDT[IDT_DESC_CNT];
/* 这里的intrEntryTable是中断入口地址，中断对应的函数的地址被存放到了这里
 * 每项是32bit的数据
 */
// 保存中断的名称
char* intrNames[IDT_DESC_CNT];
// 中断入口（汇编）
extern intrHandler intrEntryTable[IDT_DESC_CNT];
// 中断处理程序（C）
intrHandler intrHandlerTable[IDT_DESC_CNT];
// 临时
int intrCount = 0;
static void generalIntrHandler(uint8 intrVecNum)
{
    if(intrVecNum == 0x27 || intrVecNum == 0x2F)
    {
        return;
    }
    intrCount++;
    putStr("Interrupt occur!, VEC_NUM = ");
    putHex(intrVecNum);
    putStr(", COUNT = ");
    putHex(intrCount);
    cPutChar(0x07,'\n');
}

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
static void initException(void){
    for(int index = 0; index < IDT_DESC_CNT; index++){
        intrNames[index] = "Reserved";
        intrHandlerTable[index] = generalIntrHandler;
    }
    intrNames[0] = "#DE Divide Error";
    intrNames[1] = "#DB Debug Exception";
    intrNames[2] = "NMI Interrupt";
    intrNames[3] = "#BP Breakpoint Exception";
    intrNames[4] = "#OF Overflow Exception";
    intrNames[5] = "#BR BOUND Range Exceeded Exception";
    intrNames[6] = "#UD Invalid Opcode Exception";
    intrNames[7] = "#NM Device Not Available Exception";
    intrNames[8] = "#DF Double Fault Exception";
    intrNames[9] = "Coprocessor Segment Overrun";
    intrNames[10] = "#TS Invalid TSS Exception";
    intrNames[11] = "#NP Segment Not Present";
    intrNames[12] = "#SS Stack Fault Exception";
    intrNames[13] = "#GP General Protection Exception";
    intrNames[14] = "#PF Page-Fault Exception";
    intrNames[15] = "- (Intel reserved. Do not use)";
    intrNames[16] = "#MF x87 FPU Floating-Point Error";
    intrNames[17] = "#AC Alignment Check Exception";
    intrNames[18] = "#MC Machine-Check Exception";
    intrNames[19] = "#XF SIMD Floating-Point Exception";
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
    initException();
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
