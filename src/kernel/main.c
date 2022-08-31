#include "init.h"
#include "debug.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
#include "string.h"
#include "stdio.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"
#include "process.h"
#include "bitmap.h"
#include "syscall.h"
#include "syscall-init.h"
#include "stdint.h"

char* MSG_KERNEL = "[05] kernel start\n";
char buff[128];
uint16 UPA_PID;
uint16 UPB_PID = 0x64;
void kThreadA(void*);
void kThreadB(void*);
void uProcA(void);
void uProcB(void);
void kernelMain(void)
{
    printf("\n\n\n\n%s", MSG_KERNEL);
    init();
    processExecute(uProcA, "UPA");
    processExecute(uProcB, "UPB");
    intrEnable();
    memset(buff, '\0', 128);
    format(buff, "Kernel thread Main pid: 0x%x\n", (uint32)sys_getpid());
    consolePrint(buff);
    startThread("KTA", 31, kThreadA, "argA ");
    startThread("KTB", 31, kThreadB, "argB ");
    while(1);
}
void uProcA(void){
    UPA_PID = getpid();
    while(1);
}
void uProcB(void){
    UPB_PID = getpid();
    while(1);
}
void kThreadA(void* arg){
    char buffA[128];
    memset(buffA, '\0', 128);
    format(buffA, "Kernel thread A pid: 0x%x, UPA PID: 0x%x\n", (uint32)sys_getpid(), (uint32)UPA_PID);
    consolePrint(buffA);
    while(1);
}
void kThreadB(void* arg){
    char buffB[128];
    memset(buffB, '\0', 128);
    format(buffB, "Kernel thread B pid: 0x%x, UPB PID: 0x%x\n", (uint32)sys_getpid(), (uint32)UPB_PID);
    consolePrint(buffB);
    while(1);
}