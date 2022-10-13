#include "process.h"
#include "thread.h"
#include "constant.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "tss.h"
#include "console.h"
#include "string.h"
extern void intrExit(void);
/* 构建用户进程
 * 初始化上下文
 */
void startProcess(void* fileName){
    void* func = fileName;
    struct TaskStruct* currentThread = runningThread();
    currentThread->selfKernelStack += sizeof(struct ThreadStack);
    struct IntrStack* processStack = (struct IntrStack*)currentThread->selfKernelStack;
    processStack->EDI = processStack->ESP_DUMMY = 0;
    processStack->EBX = processStack->EDX = 
    processStack->ECX = processStack->EAX = 0;
    processStack->GS = 0; // 用户进程无权访问，直接设置为0即可
    processStack->DS = processStack->ES = processStack->FS = SELECTOR_USER_DATA;
    processStack->ERR_CODE = 0;
    processStack->EIP = func;
    processStack->CS = SELECTOR_USER_CODE;
    processStack->EFLAGS = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    uint32 vaddr = ((uint32)mallocAPage(PF_USER, (uint32)(USER_STACK_VADDR)) + PAGE_SIZE);
    processStack->ESP = (void*)vaddr;
    processStack->SS = (uint32)(SELECTOR_USER_STACK);
    ASSERT(currentThread->stackMagic !=  19940520);
    asm volatile("movl %0, %%esp;jmp intrExit" : : "g"(processStack): "memory");
}
/* 激活页表
 */
void activatePageDir(struct TaskStruct* pThread){
    /* 恢复页表
     * 上一次被调度的可能是进程，也可能是线程，不恢复页表的情况下，线程可能会使用进程的页表
     */
    // 若为内核线程，则需要重新填充页表为0x200000, 内核目录页表所在位置
    uint32 pageDirPaddr = 0x200000;
    if(pThread->pageDir != NULL){
        pageDirPaddr = getVaddrMapedPaddr((uint32)pThread->pageDir);
    }
    /*更新cr3,使新页表生效*/
    asm volatile("movl %0, %%cr3" : : "r" (pageDirPaddr):"memory");
}
/* 激活线程或进程的页表，更新tss中的ESP0为进程的特权级为0的栈 */
void activateProcess(struct TaskStruct* pThread){
    ASSERT(pThread != NULL);
    activatePageDir(pThread);
    if(pThread->pageDir != NULL){
        // 更新进程ESP0, 用于此进程被中断时保留上下文
        updateTssEsp0(pThread);
    }
}
/* 创建用户页目录表
 * 成功则返回页目录的虚拟地址，否则返回负一
 */
uint32* createUserPageDir(void){
    // 用户进程的页表不能让用户访问，从内核中申请内存
    uint32* pageDirVaddr = allocKernelPages(1);
    if(pageDirVaddr == NULL){
        consolePrint("Create user page dir: alloc kernel page failed.\n");
        return NULL;
    }
    /* 复制页表
     * 从768项开始至1023项是内核对应的页表
     * 为了程序共用内核，将内核对应的页表复制给用户进程
     * 创建用户页目录的时候，运行在内核中因此 0xFFFFF000会访问到内核页目录的第0个页目录表项， 0x300 * 4则是第736个项的开始位置
     */
    memcpy((uint32*)((uint32)pageDirVaddr + 0x300 * 4), (uint32*)(0xFFFFF000 + 0x300 * 4), 1024);
    // 更新页目录地址
    uint32 newPageDirPaddr = getVaddrMapedPaddr((uint32)pageDirVaddr);
    pageDirVaddr[1023] = newPageDirPaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1;
    return pageDirVaddr;
}

/* 创建用户虚拟地址位图
 * 
 */
void createUserVaddrBitmap(struct TaskStruct* userProg){
    userProg->userProgVaddrPool.vaddrStart = USER_VADDR_START;
    //char buff[128];
    //format(buff, "CYVB: 0x%x\n",(uint32)&userProg->userProgVaddrPool);
    //consolePrint(buff);
    // 计算出位图所需要的大小
    uint32 bitMapPageCount = DIV_ROUND_UP((KERNEL_VADDR_START - USER_VADDR_START)/ PAGE_SIZE / 8, PAGE_SIZE);
    userProg->userProgVaddrPool.vaddrBitmap.bits = allocKernelPages(bitMapPageCount);
    //format(buff,"P_CNG: %d, BITMAP_VADDR: 0x%x\n",bitMapPageCount, userProg->userProgVaddrPool.vaddrBitmap.bits);
    //consolePrint(buff);
    userProg->userProgVaddrPool.vaddrBitmap.length = (KERNEL_VADDR_START - USER_VADDR_START) / PAGE_SIZE / 8;
    initBitmap(&userProg->userProgVaddrPool.vaddrBitmap);
}
/* 创建用户进程*/
void processExecute(void* fileName, char* name){
    //logWarning("PE START\n");
    char buff[128];
    /* 进程的PCB由内核维护, 在内核中申请PCB需要的内存 */
    struct TaskStruct* thread = allocKernelPages(1);
    initThread(thread, name, DEFAULT_PRIO);
    createUserVaddrBitmap(thread);
    createThread(thread, startProcess, fileName);
    thread->pageDir = createUserPageDir();
    memBlockDescInit(thread->userMemBlockDescs);
    enum IntrStatus oldStatus = intrDisable();
    ASSERT(!listFind(&readyThreadList,&thread->generalTag))
    listAppend(&readyThreadList, &thread->generalTag);
    ASSERT(!listFind(&allThreadList,&thread->allListTag))
    listAppend(&allThreadList, &thread->allListTag);
    strformat(buff, "RTL: num = 0x%x, pid = 0x%x\n",listLen(&allThreadList), (uint32)thread->pid);
    //consolePrint(buff);
    setIntrStatus(oldStatus);
    //logWarning("PE END\n");
}