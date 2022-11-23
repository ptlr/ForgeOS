#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE
};
// 获取进程PID
uint32 getpid(void);
// 向控制台输出字符串
uint32 write(int32 fd, char* str, uint32 count);
// 申请size字节的内存
void* malloc(uint32 size);
// 释放指针指向的内存
void free(void* ptr);
#endif