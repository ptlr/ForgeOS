#ifndef USER_SYSCALLINIT_H
#define USER_SYSCALL_INIT_H
#include "stdint.h"
/*初始化系统调用*/
uint32 sys_getpid(void);
void syscallInit(void);
#endif