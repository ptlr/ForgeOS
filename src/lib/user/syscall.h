#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR{
    SYS_GETPID  =   0
};
// 获取进程PID
uint32 getpid(void);
#endif