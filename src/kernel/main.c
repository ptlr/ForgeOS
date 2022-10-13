#include "init.h"
#include "debug.h"
#include "printk.h"
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
extern void kernelMain(void);
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
    printkf("%s\n\n\n\n", MSG_KERNEL);
    init();
    intrEnable();
    processExecute(uProcA, "UPA");
    processExecute(uProcB, "UPB");
    startThread("KTA", 31, kThreadA, "KTA");
    startThread("KTB", 31, kThreadB, "KTB");
    while(1);
}
void kThreadA(void* arg){
    char buffer[256];
    memset(buff,'\0', 256);
    void* vaddr1 = sys_malloc(256);
    void* vaddr2 = sys_malloc(255);
    void* vaddr3 = sys_malloc(254);
    strformat(buffer, "KTA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    consolePrint(buffer);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    sys_free(vaddr1);
    sys_free(vaddr2);
    sys_free(vaddr3);
    while(1);
}
void kThreadB(void* arg){
    char buffer[256];
    memset(buff,'\0', 256);
    void* vaddr1 = sys_malloc(256);
    void* vaddr2 = sys_malloc(255);
    void* vaddr3 = sys_malloc(254);
    strformat(buffer, "KTA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    consolePrint(buffer);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    sys_free(vaddr1);
    sys_free(vaddr2);
    sys_free(vaddr3);
    while(1);
}
void uProcA(void){
    void* vaddr1 = malloc(256);
    void* vaddr2 = malloc(255);
    void* vaddr3 = malloc(254);
    printf("UPA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    free(vaddr1);
    free(vaddr2);
    free(vaddr3);
    while(1);
}
void uProcB(void){
    void* vaddr1 = malloc(256);
    void* vaddr2 = malloc(255);
    void* vaddr3 = malloc(254);
    printf("UPB VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    free(vaddr1);
    free(vaddr2);
    free(vaddr3);
    while(1);
}