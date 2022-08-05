#include "sync.h"
#include "list.h"
#include "debug.h"
#include "interrupt.h"
#include "thread.h"
#include "print.h"
/*初始信号量*/
void semaInit(struct Semaphore* sema, uint8 value){
    sema->value = value;
    listInit(&sema->waitList);
}
/*信号量down操作，请求锁相关*/
void semaDown(struct Semaphore* sema){
    // 原子操作
    enum IntrStatus oldStatus = intrDisable();
    while(sema->value == 0){ // 如果为0表示锁被其他线程持有
        //putStr("Wait Test\n");
        // 当前线程不应该在等待队列中
        if(listFind(&sema->waitList, &runningThread()->generalTag)){
            PANIC("Sema down: thread blocked has been in wait list\n");
        }
        // 加入该锁的等待队列，之后阻塞自己
        listAppend(&sema->waitList, &runningThread()->generalTag);
        threadBlock(TASK_BLOCKED);
    }
    // 如果信号量为1，执行后面的代码后线程获得锁
    ASSERT(sema->value == 1);
    sema->value--;
    ASSERT(sema->value == 0);
    // 恢复中断状态
    setIntrStatus(oldStatus);
}
/*信号up操作，释放锁相关*/
void semaUp(struct Semaphore* sema){
    // 所有操作在光终端后
    enum IntrStatus oldStatus = intrDisable();
    if(!listIsEmpty(&sema->waitList)){
        struct TaskStruct* blockedThread = elem2entry(struct TaskStruct, generalTag, listPop(&sema->waitList));
        threadUnblock(blockedThread);
    }
    sema->value++;
    ASSERT(sema->value == 1);
    setIntrStatus(oldStatus);
}
/*初始化锁*/
void lockInit(struct Lock* lock){
    lock->holder = NULL;
    lock->holderRepeatNum = 0;
    semaInit(&lock->seamphore, 1);
}
/*申请锁*/
void lockAcquire(struct Lock* lock){
    if(lock->holder != runningThread()){
        semaDown(&lock->seamphore);
        lock->holder = runningThread();
        ASSERT(lock->holderRepeatNum == 0);
    }else{
        intrDisable();
        putStr("HRN++\n");
        intrEnable();
        lock->holderRepeatNum++;
    }
}
/*释放锁*/
void lockRelease(struct Lock* lock){
    if(lock->holderRepeatNum > 1){
        lock->holderRepeatNum--;
        return;
    }
    ASSERT(lock->holderRepeatNum == 0);
    lock->holder = NULL;
    lock->holderRepeatNum = 0;
    semaUp(&lock->seamphore);
}