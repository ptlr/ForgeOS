#include "fork.h"
#include "stdint.h"
#include "string.h"
#include "thread.h"
#include "debug.h"
#include "process.h"
#include "file.h"
#include "interrupt.h"
// 为子进程拷贝父进程的PCB
static int32 copyPCB(struct TaskStruct* childThread, struct TaskStruct* parentThread){
    // 先整个复制，再处理个别数据
    memcpy(childThread, parentThread, PAGE_SIZE);
    childThread->pid = forkPid();
    childThread->elapsedTicks = 0;
    childThread->status = TASK_READY;
    childThread->ticks = childThread->priority;
    childThread->ppid = parentThread->pid;
    childThread->generalTag.prev = childThread->generalTag.next = NULL;
    childThread->allListTag.prev = childThread->allListTag.next = NULL;
    // 初始化内存描述符
    memBlockDescInit(childThread->userMemBlockDescs);
    // 复制父进程的虚拟地址池的位图
    uint32 bitmapPageCnt = DIV_ROUND_UP((KERNEL_VADDR_START - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    void* vaddrBitmap = allocKernelPages(bitmapPageCnt);
    // 更新子进程虚拟地址位图
    memcpy(vaddrBitmap, childThread->userProgVaddrPool.vaddrBitmap.bits, bitmapPageCnt * PAGE_SIZE);
    childThread->userProgVaddrPool.vaddrBitmap.bits = vaddrBitmap;
    ASSERT(strlen(childThread->name) < 27);
    strcat(childThread->name, "_fork");
    return 0;
}

// 复制子进程的进程体（代码、数据）以及用户栈
static void copyBodyStack3(struct TaskStruct* childThread, struct TaskStruct* parentThread, void* bufferPage){
    uint8* vaddrBitmap = parentThread->userProgVaddrPool.vaddrBitmap.bits;
    uint32 bitmapByteCnt = parentThread->userProgVaddrPool.vaddrBitmap.length;
    uint32 vaddrStart = parentThread->userProgVaddrPool.vaddrStart;
    uint32 byteIndex = 0;
    uint32 bitIndex = 0;
    uint32 progVaddr = 0;
    // 在父进程中查找已有数据项
    while (byteIndex < bitmapByteCnt)
    {
        if(vaddrBitmap[byteIndex]){
            bitIndex = 0;
            while (bitIndex < 8)
            {
                progVaddr = (byteIndex * 8 + bitIndex) * PAGE_SIZE + vaddrStart;
                //printkf("FORK:: PROG_VADDR:0x%08x\n", progVaddr);
                if(!((BITMAP_MASK << bitIndex) & vaddrBitmap[byteIndex])){
                    bitIndex++;
                    continue;
                }
                // 1、将父进程的数据复制到内核缓冲区
                memcpy(bufferPage, (void*)progVaddr, PAGE_SIZE);
                // 2、切换至子进程进程页表
                activatePageDir(childThread);
                // 3、为子进程申请物理页并添加到对应的虚拟地址上
                forkMapVaddr(PF_USER, progVaddr);
                // 4、复制数据到子进程中
                memcpy((void*)progVaddr, bufferPage, PAGE_SIZE);
                // 5、切换至父进程进程页表
                activatePageDir(parentThread);
                bitIndex++;
            }
        }
        byteIndex++;
    } 
}

// 为子进程构建threadStack和修改放回栈
static int32 buildChildStack(struct TaskStruct* childThread){
    // 1、使子进程返回的pid为0，表示该进程属于子进程
    struct IntrStack* intr0Stack = (struct IntrStack*)((uint32)childThread + PAGE_SIZE - sizeof(struct IntrStack));
    intr0Stack->EAX = 0;
    // 2、为switch2构建struct ThreadStack
    uint32* retAddrInThreadStack = (uint32*)intr0Stack - 1;
    // 此处省去梳理栈中的关系, 也省去了下面两句调试语句
    // ebp在ThreadStack中的地址就是当时的esp(0级栈栈顶)
    uint32* ebpPtrInThreadStack  = (uint32*)intr0Stack - 5;
    // 设置返回地址为中断退出
    *retAddrInThreadStack = (uint32)intrExit;
    // 把构建的ThreadStack栈顶作为switch2恢复数据时的栈顶
    childThread->selfKernelStack = ebpPtrInThreadStack;
    return 0;
}

// 更新inode打开次数
static void updateInodeOpenCnt(struct TaskStruct* thread){
    int32 lfd = 3;
    int32 gfd = 0;
    while(lfd < PROC_MAX_OPEN_FILE_NUM){
        gfd = thread->fdTable[lfd];
        if(gfd != -1){
            fileTable[gfd].fdInode->opentCnt++; 
        }
        lfd++;
    }
}
// 拷贝父进程本身所占的资源给子进程
static int32 copyProcess(struct TaskStruct* childThread, struct TaskStruct* parentThread){
    // 内核缓冲区
    void* bufferPage = allocKernelPages(1);
    if(bufferPage == NULL){
        return -1;
    }
    // 1、复制父进程PCB、虚拟地址位图、内核栈
    if(copyPCB(childThread, parentThread) == -1) return -1;
    // 2、为子进程创建页表，包括内核空间
    childThread->pageDir = createUserPageDir();
    if(childThread->pageDir == NULL) return -1;
    // 3、复制父进程体以及用户栈给子进程
    copyBodyStack3(childThread, parentThread, bufferPage);
    // 4、构建子进程以及修改返回值PID
    buildChildStack(childThread);
    // 5、更新inode文件打开次数
    updateInodeOpenCnt(childThread);
    mfreePage(PF_KERNEL, bufferPage, 1);
    return 0;
}

// fork子进程，内核线程不可直接调用
int16 sysFork(void){
    struct TaskStruct* parentThread = runningThread();
    struct TaskStruct* childThread = allocKernelPages(1);
    if(childThread == NULL) return -1;
    ASSERT(INTR_OFF == getIntrStatus() && parentThread->pageDir != NULL);
    if(copyProcess(childThread, parentThread) == -1) return -1;
    // 将子线程添加到就绪队列
    listAppend(&readyThreadList, &childThread->generalTag);
    // 将子线程添加到所有任务队列
    listAppend(&allThreadList, &childThread->allListTag);
    // 父进程返回子进程PID
    return childThread->pid;
}