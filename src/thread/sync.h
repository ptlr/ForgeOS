#ifndef THREAD_SYNC_H
#define THREAD_SYNC_H

#include "list.h"
#include "stdint.h"
//#include "thread.h"

/* 信号量结构
 * 
 */
struct Semaphore{
    uint8 value;
    struct List waitList;
};
/*
 * 锁结构 
 */
struct Lock{
    // 锁的名称
    char name[64];
    // 锁的持有者
    struct TaskStruct* holder;
    // 二元信号量
    struct Semaphore seamphore;
    // 锁的持有者重复申请锁的次数
    uint32 holderRepeatNum;
};

/*初始信号量*/
void semaInit(struct Semaphore* sema, uint8 value);
/*信号量down操作，请求锁相关*/
void semaDown(struct Semaphore* sema);
/*信号up操作，释放锁相关*/
void semaUp(struct Semaphore* sema);
/*初始化锁*/
void lockInit(struct Lock* lock, char* name);
/*申请锁*/
void lockAcquire(struct Lock* lock);
/*释放锁*/
void lockRelease(struct Lock* lock);
#endif