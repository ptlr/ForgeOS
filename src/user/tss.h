#ifndef USER_TSS_H
#define USER_TSS_H
#include "stdint.h"
#include "thread.h"
/*任务状态段Tss*/
struct TSS{
    uint32 backLink;
    uint32* ESP0;
    uint32 SS0;
    uint32* ESP1;
    uint32 SS1;
    uint32* ESP2;
    uint32 SS2;
    uint32 CR3;
    uint32 (*EIP) (void);
    uint32 EFLAGS;
    uint32 EAX;
    uint32 ECX;
    uint32 EDX;
    uint32 EBX;
    uint32 ESP;
    uint32 EBP;
    uint32 ESI;
    uint32 EDI;
    uint32 ES;
    uint32 CS;
    uint32 SS;
    uint32 DS;
    uint32 FS;
    uint32 LDT;
    uint32 trace;
    uint32 IO_BASE;
};
/* 更新tss中ESP0字段的值为pthread的0级栈 */
void updateTssEsp0(struct TaskStruct* pthread);
/* 初始化TSS */
void initTss(int (* step)(void));
#endif