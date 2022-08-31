#include "syscall-init.h"
#include "print.h"
#include "syscall.h"
#include "thread.h"
#define SYSCALL_COUNT  32
typedef void* syscall;
syscall syscallTable[SYSCALL_COUNT];
/*获取线程的进程PID*/
uint32 sys_getpid(void){
    return runningThread()->pid;
}
/*初始化系统调用*/
void syscallInit(void){
    putStr("[14] init system call\n");
    syscallTable[SYS_GETPID] = sys_getpid;
}