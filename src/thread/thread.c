#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "constant.h"
#include "memory.h"
#include "stdio.h"
/*由kernelThread去执行function(funcArg)*/
static void kernelThread(thread_func* func, void* funcArg){
    func(funcArg);
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
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    pthread->selfKernelStack = (uint32*)((uint32)pthread + PAGE_SIZE);
    // 自定义的魔数：用于检查栈溢出
    pthread->stackMagic = 0x19940520;
}

/* 创建一个名为name,优先级为prio的线程
 */

struct TaskStruct* startThread(char* name, int prio, thread_func func, void* funcArg){
    // PCB都放在内核空间
    struct TaskStruct* thread = allocKernelPages(1);
    printf("THREAD PCB VADDR: 0x%x\n", (uint32)thread);
    initThread(thread, name, prio);
    createThread(thread, func, funcArg);
    /*
     *
     */
    printf("<<ST>>\n");
    printf("SKS_VADDR = 0x%x\n", thread->selfKernelStack);
    //while(1);
    asm volatile("movl %0, %%esp;\
    pop %%ebp;\
    pop %%ebx;\
    pop %%edi;\
    pop %%esi;\
    ret;": :"g"(thread->selfKernelStack):"memory");
    return thread;
}
