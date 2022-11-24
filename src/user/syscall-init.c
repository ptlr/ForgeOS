#include "syscall-init.h"
#include "string.h"
#include "printk.h"
#include "syscall.h"
#include "thread.h"
#include "console.h"
#include "fs.h"
#include "fork.h"

#define SYSCALL_COUNT  32
typedef void* syscall;
syscall syscallTable[SYSCALL_COUNT];
/*获取线程的进程PID*/
uint32 sys_getpid(void){
    return runningThread()->pid;
}
/*初始化系统调用*/
void syscallInit(int (* step)(void)){
    printkf("[%02d] init system call\n", step());
    syscallTable[SYS_GETPID] = sys_getpid;
    syscallTable[SYS_WRITE]  = sysWrite;
    syscallTable[SYS_MALLOC] = sys_malloc;
    syscallTable[SYS_FREE] = sys_free;
    syscallTable[SYS_FORK] = sysFork;
    syscallTable[SYS_READ] = sysRead;
    syscallTable[SYS_PUTCHAR] = putChar;
    syscallTable[SYS_CLEAR] = sysClsScreen;
    syscallTable[SYS_GETCWD] = sysGetCwd;
    syscallTable[SYS_OPEN] = sysOpen;
    syscallTable[SYS_CLOSE] = sysClose;
    syscallTable[SYS_LSEEK] = sysLSeek;
    syscallTable[SYS_UNLINK] = sysUnlink;
    syscallTable[SYS_MKDIR] = sysMkdir;
    syscallTable[SYS_OPENDIR] = sysOpendir;
    syscallTable[SYS_CLOSEDIR] = sysOpendir;
    syscallTable[SYS_READDIR] = sysReadDirEntry;
    syscallTable[SYS_REWINDDIR] = sysRewinddir;
    syscallTable[SYS_STAT] = sysStat;
    syscallTable[SYS_PS] = sysPs;
}