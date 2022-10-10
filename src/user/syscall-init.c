#include "syscall-init.h"
#include "string.h"
#include "print.h"
#include "syscall.h"
#include "thread.h"
#include "console.h"

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
    syscallTable[SYS_WRITE]  = sys_write;
    syscallTable[SYS_MALLOC] = sys_malloc;
    syscallTable[SYS_FREE] = sys_free;
}
uint32 sys_write(char* str){
    consolePrint(str); 
    return strlen(str);
}