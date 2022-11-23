#include "block.h"
#include "file.h"
#include "printk.h"
#include "bitmap.h"

// 在inode中分配一个块, 成功返回块起始LBA,失败返回-1
int32 blockAlloc(struct Partition* part){
    uint32 blockLBA = blockBitmapAlloc(part);
    if(blockLBA == -1){
        printk("FS_ALLOC_BLOCK: alloc block bitmap failed!\n");
        return -1;
    }
    uint32 blockBitmapIndex = blockLBA - part->superBlock->dataStartLba;
    bitmapSync(part, blockBitmapIndex, BLOCK_BITMAP);
    return blockLBA;
}
// 释放blockLBA对应的块
void blockRelease(struct Partition* part, int32 blockLBA){
    uint32 blockBitmapIndex = blockLBA - part->superBlock->dataStartLba;
    setBitmap(&part->blockBitmap, blockBitmapIndex, 0);
    bitmapSync(part, blockBitmapIndex, BLOCK_BITMAP);
}