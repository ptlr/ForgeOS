#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_PUTCHAR,
    SYS_CLEAR
};
// 获取进程PID
uint32 getpid(void);
// 向控制台输出字符串
uint32 write(int32 fd, char* str, uint32 count);
// 申请size字节的内存
void* malloc(uint32 size);
// 释放指针指向的内存
void free(void* ptr);
// 创建子进程
int16 fork(void);
// 显示一个字符
void putchar(char ch);
// 清空屏幕
void clsScreen(void);
#endif