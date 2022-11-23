#ifndef USER_FORK_H
#define USER_FORK_H
#include "stdint.h"
// 导入中断退出函数
extern void intrExit(void);
// fork子进程，内核线程不可直接调用
int16 sysFork(void);
#endif