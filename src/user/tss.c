#include "tss.h"
#include "thread.h"
#include "constant.h"
#include "string.h"
#include "print.h"
// 最开始的TSS
static struct TSS tss;
/* 更新tss中ESP0字段的值为pthread的0级栈 */
void updateTssEsp0(struct TaskStruct* pthread){
    tss.ESP0    = (uint32*)((uint32)pthread + PAGE_SIZE);
}
static struct GdtDesc makeGdtDesc(uint32* descAddr, uint32 limit, uint8 attrLow, uint8 attrHigh){
    uint32 descBase = (uint32)descAddr;
    struct GdtDesc desc;
    desc.limitLowWord = limit & 0x0000FFFF;
    desc.baseLowWord = descBase & 0x0000FFF;
    desc.baseMidByte = (descBase & 0x00FF0000) >> 16;
    desc.attrLowByte = (uint8)attrLow;
    desc.LimiHighAttrHigh = (((limit & 0x000F0000) >> 16) + (uint8)(attrHigh));
    desc.baseHighByte = descBase >> 24;
    return desc;
}
/* 初始化TSS */
void initTss(){
    putStr("[13] init TSS\n");
    uint32 tssSize = sizeof(tss);
    // 清空TSS
    memset(&tss, 0, tssSize);
    tss.SS0  = SELECTOR_KERNEL_STACK;
    /* 位图大小设置成大于等于TSS长度减1时，表示没有位图 */
    tss.IO_BASE = tssSize;
    /* 获取GDT地址
     * 描述：前面为TSS和用户代码段和用户数据段留的空白位置，此处可获取位置后操作
     */
    uint64 gdtOperand = 0;
    asm volatile("sgdt %0"::"m"(gdtOperand));
    uint32 gdtAddr = (uint32)(gdtOperand >> 16);
    //uint32 gdtLimit = (uint32)(gdtOperand & 0x000000000000FFFF);
    // 构建TSS对应的描述符
    /* kernel.asm中第5个位置开始存放，TSS,用户代码段， 用户数据段
     * 其中，TSS地址计算方式为：index * 8
     */
    uint32 descAddr = gdtAddr + 4 * 8;
    *((struct GdtDesc*)descAddr) = makeGdtDesc((uint32*)&tss, tssSize - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
    // 构建用户代码段对应的描述符
    descAddr += 8;
    *((struct GdtDesc*)descAddr) = makeGdtDesc((uint32*)0, 0xFFFFF, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    // 构建用户数据段对应的描述符
    descAddr += 8;
    *((struct GdtDesc*)descAddr) = makeGdtDesc((uint32*)0, 0xFFFFF, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

    // 重新加载GDT, 由于预留空间，此处不用重新计算GTD地址数据
    asm volatile("lgdt %0" : : "m" (gdtOperand));
    // 加载操作系统第一个TSS
    asm volatile("ltr %w0" : : "r" (SELECTOR_TSS));
}