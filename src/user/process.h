#ifndef USER_PROCESS_H
#define USER_PROCESS_H

#include "thread.h"

// 默认优先级
#define DEFAULT_PRIO    31 

#define USER_STACK_VADDR    0xC0000000 - 0x1000
// 编程技巧：这里传入的VALUE和STEP肯能会是一个表达式，使用小括号可以保证优先级
#define DIV_ROUND_UP(VALUE, STEP) ((VALUE + STEP - 1) / (STEP))
/* 构建用户进程
 * 初始化上下文
 */
void startProcess(void* fileName);

/* 激活页表*/
void activatePageDir(struct TaskStruct* pThread);

/* 激活线程或进程的页表，更新tss中的ESP0为进程的特权级为0的栈 */
void activateProcess(struct TaskStruct* pThread);

/* 创建用户页目录表
 * 成功则返回页目录的虚拟地址，否则返回负一
 */
uint32* createUserPageDir(void);

/* 创建用户虚拟地址位图
 * 
 */
void createUserVaddrBitmap(struct TaskStruct* userProg);
/* 创建用户进程*/
void processExecute(void* fileName, char* name);
#endif