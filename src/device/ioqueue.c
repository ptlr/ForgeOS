#include "ioqueue.h"
#include "debug.h"
#include "interrupt.h"
/* 初始化队列 */
void initIoqueue(struct ioqueue* ioq){
    lockInit(&ioq->lock);
    ioq->producer = NULL;
    ioq->consumer = NULL;
    ioq->head = 0;
    ioq->tail = 0;
}
/* 返回缓冲区的下一个位置 */
static int32 ioqNextPos(int32 pos){
    // 环形，取模会让位置一直大于0小于BUFFER_SIZE - 1
    return (pos + 1) % BUFFER_SIZE;
}
/* 判断队列是否已满 */
bool ioqIsFull(struct ioqueue* ioq){
    ASSERT(getIntrStatus() == INTR_OFF);
    return ioqNextPos(ioq->head) == ioq->tail;
}
/* 判断队列是否为空 */
static bool ioqIsEmpty(struct ioqueue* ioq){
    ASSERT(getIntrStatus() == INTR_OFF);
    return ioq->head == ioq->tail;
}
/* 使当前消费者或生产者在缓冲区上等待 */
static void ioqWait(struct TaskStruct** waiter){
    ASSERT(*waiter == NULL && waiter != NULL);
    *waiter = runningThread();
    threadBlock(TASK_BLOCKED);
}
/* 唤醒waiter */
static void ioqWakeup(struct TaskStruct** waiter){
    ASSERT(*waiter != NULL);
    threadUnblock(*waiter);
    *waiter = NULL;
}
/* 消费者从队列中取出一个字符 */
char ioqGetChar(struct ioqueue* ioq){
    ASSERT(getIntrStatus() == INTR_OFF);
    while(ioqIsEmpty(ioq)){
        lockAcquire(&ioq->lock);
        ioqWait(&ioq->consumer);
        lockRelease(&ioq->lock);
    }
    char byte = ioq->buffer[ioq->tail];
    ioq->tail = ioqNextPos(ioq->tail);
    // 唤醒生产者
    if(ioq->producer != NULL)  ioqWakeup(&ioq->producer);
    return byte;
}
/* 生产者往队列中存一个字符 */
void ioqPutChar(struct ioqueue* ioq, char ch){
    ASSERT(getIntrStatus() == INTR_OFF);
    while(ioqIsFull(ioq)){
        lockAcquire(&ioq->lock);
        ioqWait(&ioq->producer);
        lockRelease(&ioq->lock);
    }
    ioq->buffer[ioq->head] = ch;
    ioq->head = ioqNextPos(ioq->head);
    if(ioq->consumer != NULL) ioqWakeup(&ioq->consumer);
}