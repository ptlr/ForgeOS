#ifndef FS_BLOCK_H
#define FS_BLOCK_H
#include "stdint.h"
#include "ide.h"
#include "inode.h"
#define MAX_BLOCK_NUM
struct BlockInfo{
    int32 blockIndex;
    int32 blockLBA;
};
// 在inode中分配一个块, 成功返回块起始LBA,失败返回-1
int32 blockAlloc(struct Partition* part);
// 释放blockLBA对应的块
void blockRelease(struct Partition* part, int32 blockLBA);
#endif