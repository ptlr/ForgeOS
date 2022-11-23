#ifndef THREAD_THREAD_H
#define THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "memory.h"

// 每个任务可以打开的最大文件数
#define PROC_MAX_OPEN_FILE_NUM  8

// 导出就绪队列
extern struct List readyThreadList;
// 导出所有任务队列
extern struct List allThreadList;

/*
 * 线程需要执行的函数
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
    uint32 EBX;
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
    // 线程ID
    int16 pid;
    enum TaskStatus status;
    uint8 priority;
    char name[32];
    // 每次在线程上运行的滴答数
    uint8 ticks;
    // 此任务占用了CPU多少个滴答数，即运行了多长时间
    uint32 elapsedTicks;
    // 在一般队列里的节点(没有数据，无需初始化数据，下同)
    struct ListElem generalTag;
    // 位于allList中的节点
    struct ListElem allListTag;
    // 进程自己页表的虚拟地址
    uint32* pageDir;
    // 用户进程的虚拟地址
    struct VaddrPool userProgVaddrPool;
    // 用户进程内存块描述符
    struct MemBlockDesc userMemBlockDescs[MEM_BLOCK_TYPE_COUNT];
    // 文件描述符数组
    int32 fdTable[PROC_MAX_OPEN_FILE_NUM];
    // 以根目录作为默认目录
    uint32 cwdInodeNum;
    // 魔术，用于检测栈边界是否溢出
    uint32 stackMagic;
};
/* 向外部导出符号
 * ToDo::目前意义不明
 */
extern struct ListElem* generalTag;
// 用于初始化线程环境
void initThreadEnv(int (* step)(void));
// 导入外部函数
extern void switch2(struct TaskStruct* currentThread, struct TaskStruct* nextThread);
//获取当前线程的PCB
struct TaskStruct* runningThread(void);
void createThread(struct TaskStruct* pthread, thread_func func, void* funcArg);
void initThread(struct TaskStruct* pthread, char* name, int prio);
struct TaskStruct* startThread(char* name, int prio, thread_func func, void* funcArg);
// 任务调度函数
void schedule(void);
/* 当前线程将阻塞自己，并将状态设置为status */
void threadBlock(enum TaskStatus status);
/* 将线程解除阻塞，并将状态设置为status */
void threadUnblock(struct TaskStruct* thread);
// 主动让出CPU让其他线程运行
void threadYeild(void);
#endif