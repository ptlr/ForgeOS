#ifndef DEVICE_IOQUEUE_H
#define DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFFER_SIZE 64
/* 环型队列 */
struct ioqueue{
    // 生产者消费问题
    struct Lock lock;
    /*
     * 生产者，缓冲区不为空时会向继续存放数据
     */
    struct TaskStruct* producer;
    /*
     * 消费者，缓冲区不为空时，会继续取出数据
     * 否则就休眠
     */
    struct TaskStruct* consumer;
    char buffer[BUFFER_SIZE];   // 缓冲区
    int32 head; // 队首，数据从队首处写入
    int32 tail; // 队尾，数据从队尾处读取
    
};
/* 初始化队列 */
void initIoqueue(struct ioqueue* ioq);
/* 判断队列是否已满 */
bool ioqIsFull(struct ioqueue* ioq);
/* 判断队列是否为空 */
//bool ioqIsEmpty(struct ioqueue* ioq);
/* 消费者从队列中取出一个字符 */
char ioqGetChar(struct ioqueue* ioq);
/* 生产者往队列中存一个字符 */
void ioqPutChar(struct ioqueue* ioq, char ch);
#endif