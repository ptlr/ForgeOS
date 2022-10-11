#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "constant.h"
#include "memory.h"
#include "stdio.h"
#include "interrupt.h"
#include "debug.h"
#include "process.h"
#include "console.h"

// 主线程
struct TaskStruct* kernelMainThread;
// 就绪队列
struct List readyThreadList;
// 所有任务队列
struct List allThreadList;
// 定义
struct ListElem* generalTag;
/*由kernelThread去执行function(funcArg)*/
static void kernelThread(thread_func* func, void* funcArg){
    // 执行前开中断，避免其他线程无法被调度
    intrEnable();
    func(funcArg);
}
// 分配PID
struct Lock pidLock;
static int16 allocatePid(void){
    static int16 nextPid = 0;
    lockAcquire(&pidLock);
    nextPid++;
    lockRelease(&pidLock);
    return nextPid;
}
/* 创建线程：
 * 把被执行的函数和它的参数保存到ThreadStack对应的位置
 */
void createThread(struct TaskStruct* pthread, thread_func func, void* funcArg){
    // 预留中断栈
    pthread->selfKernelStack -= sizeof(struct IntrStack);
    // 预留线程栈空间
    pthread->selfKernelStack -= sizeof(struct ThreadStack);

    struct ThreadStack* kthreadStack = (struct ThreadStack*) pthread->selfKernelStack;
    // kernelThread定义在头部
    kthreadStack->EIP = kernelThread;
    kthreadStack->func = func;
    kthreadStack->funcArg = funcArg;
    kthreadStack->EBP = 0;
    kthreadStack->ESI = 0;
}

/* 初始化线程基本信息：
 * 线程名、优先级
 */
void initThread(struct TaskStruct* pthread, char* name, int prio){
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->pid = allocatePid();
    if(pthread == kernelMainThread){
        pthread->status =  TASK_RUNNING;
    }else{
        pthread->status =  TASK_READY;
    }
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsedTicks = 0;
    pthread->pageDir = NULL;
    pthread->selfKernelStack = (uint32*)((uint32)pthread + PAGE_SIZE);
    // 自定义的魔数：用于检查栈溢出
    pthread->stackMagic = 0x19940520;
}

/* 创建一个名为name,优先级为prio的线程
 */

struct TaskStruct* startThread(char* name, int prio, thread_func func, void* funcArg){
    // PCB都放在内核空间
    struct TaskStruct* thread = allocKernelPages(1);
    initThread(thread, name, prio);
    createThread(thread, func, funcArg);
    // 确保线程不在就绪队列中
    ASSERT(!listFind(&readyThreadList, &thread->generalTag));
    listAppend(&readyThreadList, &thread->generalTag);
    // 确保不在所有队列中
    ASSERT(!listFind(&allThreadList, &thread->allListTag));
    listAppend(&allThreadList, &thread->allListTag);
    return thread;
}
static void implementMainThread(void){
    /* kernelMainThread早已经开始运行
     * 同时把它的PCB预留到0xC01FF000~0xC01FFFFF
     * 因此无需分配新的页作为PCB
     */
    kernelMainThread = runningThread();
    initThread(kernelMainThread, "Forge Kernel",31);
    // 当前线程不在ready队列中，加入到allThreadList中
    ASSERT(!listFind(&allThreadList, &kernelMainThread->allListTag));
    listAppend(&allThreadList, &kernelMainThread->allListTag);
}
struct TaskStruct* runningThread(void){
    uint32 esp;
    asm volatile("mov %%esp, %0" : "=g" (esp));
    // esp的整数部分即PCB的起始地址
    return (struct TaskStruct*)(esp & 0xFFFFF000);
}
void schedule(){
    ASSERT(getIntrStatus() == INTR_OFF);
    struct TaskStruct* current = runningThread();
    if(current->status == TASK_RUNNING){
        ASSERT(!listFind(&readyThreadList, &current->generalTag));
        listAppend(&readyThreadList, &current->generalTag);
        current->ticks = current->priority;
        current->status = TASK_READY;
    }else{
        //while(1);
        /* 如果此线程需要某件事发生后才继续运行，不需要加入到就绪队列 */
    }
    // 调度其他任务， ready队列不应该为空
    ASSERT(!listIsEmpty(&readyThreadList));
    struct ListElem* threadTag = listPop(&readyThreadList);
    struct TaskStruct* next = elem2entry(struct TaskStruct, generalTag, threadTag);
    next->status =  TASK_RUNNING;
    activateProcess(next);
    switch2(current, next);
}
void initThreadEnv(void){
    printk("[10] init thread env\n");
    listInit(&readyThreadList);
    listInit(&allThreadList);
    lockInit(&pidLock, "PidLock");
    implementMainThread();
}

/* 当前线程将阻塞自己，并将状态设置为status */
void threadBlock(enum TaskStatus status){
    ASSERT(status == TASK_BLOCKED || status == TASK_WAITING || status == TASK_HANGING);
    // 关中断
    enum IntrStatus oldStatus = intrDisable();
    struct TaskStruct* thread = runningThread();
    thread->status = status;
    //printf("Thread Status TB: 0x%d\n", (uint32)thread->status);
    // 将当前线程换下处理器
    schedule();
    setIntrStatus(oldStatus);
}
/* 将线程解除阻塞，并将状态设置为status */
void threadUnblock(struct TaskStruct* thread){
    enum IntrStatus oldStatus = intrDisable();
    ASSERT(thread->status == TASK_BLOCKED || thread->status == TASK_WAITING || thread->status == TASK_HANGING);
    if(thread->status != TASK_READY){
        if(listFind(&readyThreadList, &thread->generalTag)){
            PANIC("Thread unblock: blocked thread in ready list\n");
        }
        // 将唤醒的线程放到队列的最前面，使其尽快得到调度
        listPush(&readyThreadList, &thread->generalTag);
        thread->status = TASK_READY;
    }else{
    }
    setIntrStatus(oldStatus);
}