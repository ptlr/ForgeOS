#ifndef DEVICE_IDE_H
#define DEVICE_IDE_H

#include "stdint.h"
#include "sync.h"
#include "list.h"
// Disk前置声明
struct IdeChannel;
/* 分区结构 */
struct Partition{
    uint32 startLBA;            // 起始扇区
    uint32 sectorCount;         // 扇区数
    struct Disk* disk;          // 属于哪个硬盘 
    struct ListElem partTag;    // 分区在队列中的标记
    char name[16];              // 分区名称
    // ToDo
};
/* 硬盘结构 */
struct Disk{
    char name[8];                       // 硬盘名称
    struct IdeChannel* channel;         // 磁盘属于哪个IDE通道
    uint8 devNum;                       // 主盘0，从盘1
    struct Partition primaryParts[4];   // 主分区最多4个
    struct Partition logicParts[8];     // 支持最多8个逻辑分区
};
/* ATA通道结构 */
struct IdeChannel{
    char name[8];       // ATA通道的名称
    uint16 basePort;    // 通道的起始端口号
    uint8 irqNum;       // 该通道使用的终端号
    struct Lock lock;   // 通道锁
    bool waitingIrq;    // 表示等待通道的中断
    struct Semaphore seamphore; // 用于阻塞、唤醒驱动程序
    struct Disk devices[2];     // 一个通道上有两个硬盘
};
// 支持的功能

// 初始化数据
void initIDE(int (* step)(void));
#endif