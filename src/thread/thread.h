#ifndef __THREAD_H
#define __THREAD_H
#include "stdint.h"
/*
 *
 */
typedef void thread_func(void*);

/*进程或线程的状态*/
enum TaskStatus{
    TASK_READY,
    TASK_RUNNING, 
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};
/* 中断栈
 */
struct IntrStack{
    uint32 vecNum;
    uint32 EDI;
    uint32 ESi;
    uint32 EBP;
    uint32 ESP_DUMMY;
    uint32 RBX;
    uint32 EDX;
    uint32 ECX;
    uint32 EAX;
    uint32 GS;
    uint32 FS;
    uint32 ES;
    uint32 DS;
    /*CPU从低特权级进入高特权级时压入*/
    uint32 ERR_CODE;
    void (*EIP) (void);
    uint32 CS;
    uint32 EFLAGS;
    void* ESP;
    uint32 SS;
};
/*线程自己的栈*/
struct ThreadStack{
    uint32 EBP;
    uint32 EBX;
    uint32 EDI;
    uint32 ESI;

    void (*EIP) (thread_func* func, void* funcArg);
    void (*unusedRetaddr);
    // 由kernel_thread所调用的函数名
    thread_func* func;
    // 由kernel_thread所调用的函数所调用的参数
    void* funcArg;
};
/*进程或线程的PCB*/
struct TaskStruct{
    // 个内核线程都用自己的内核栈
    uint32* selfKernelStack;
    enum TaskStatus status;
    uint8 priority;
    char name[16];
    uint32 stackMagic;
};
void createThread(struct TaskStruct* pthread, thread_func func, void* funcArg);
void initThread(struct TaskStruct* pthread, char* name, int prio);
struct TaskStruct* startThread(char* name, int prio, thread_func func, void* funcArg);
#endif