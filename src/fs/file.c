#include "file.h"
#include "thread.h"
#include "ide.h"
#include "fs.h"
#include "bitmap.h"
#include "printk.h"
#include "constant.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "dir.h"
#include "interrupt.h"

/* 打开的文件表 */
struct File fileTable[MAX_FILE_OPEN_NUM];

// 在fileTable中查找空位并返回index，没有空位时返回-1
int32 getFileTableIndex(void){
    // 跳过标准输入输出以及标准错误
    // 疑问： for循环是否会在多线程环境中出现问题？
    for(uint32 index = 3; index < MAX_FILE_OPEN_NUM; index++){
        if(fileTable[index].fdInode == NULL) return index;
    }
    // 执行到这里时标识没有空位
    return -1;
}

// 将全局文件描述符下标安装到任务自己的PCB中，成功返回任务自己的FD index,失败返回-1
int32 installFdInPCB(int32 fileTableIndex){
    uint32 pcbFdIndex = 3;  // 跳过标准输入、标准输出、标准错误
    struct TaskStruct* currentTask = runningThread();
    while(pcbFdIndex < PROC_MAX_OPEN_FILE_NUM){
        if(currentTask->fdTable[pcbFdIndex] == -1){ // 表示可用
            currentTask->fdTable[pcbFdIndex] = fileTableIndex;
            break;
        }
        pcbFdIndex++;
    }
    if(pcbFdIndex == PROC_MAX_OPEN_FILE_NUM) return -1;
    return pcbFdIndex;
}

// 分配一个inode并返回inode号

int32 inodeBitmapAlloc(struct Partition* part){
    int32 bitIndex = scanBitmap(&part->inodeBitmap, 1);
    if(bitIndex == -1) return bitIndex;
    setBitmap(&part->inodeBitmap, bitIndex, 1);
    return bitIndex;
}

// 分配一个扇区，返回其扇区地址(LBA)
// ToDo: 这里一个块的大小是512byte,即一个扇区
int32 blockBitmapAlloc(struct Partition* part){
    int32 bitIndex = scanBitmap(&part->blockBitmap, 1);
    if (bitIndex == -1) return bitIndex;
    setBitmap(&part->blockBitmap, bitIndex, 1);
    return (part->superBlock->dataStartLba + bitIndex);
}

// 将内存中bitmap的bitIndex位所在的512字节更新后写入硬盘
void bitmapSync(struct Partition* part, uint32 bitIndex, uint8 bitmap){
    uint32 offsetSector = bitIndex / 4096;
    uint32 offsetSize = offsetSector * BLOCK_SIZE;
    uint32 sectorLBA;
    uint8* bitmapOffset;
    switch (bitmap)
    {
        case INODE_BITMAP:
            sectorLBA = part->superBlock->inodeBitmapLba + offsetSector;
            bitmapOffset = part->inodeBitmap.bits + offsetSize;
            break;
        case BLOCK_BITMAP:
            sectorLBA = part->superBlock->blockBitmapLba + offsetSector;
            bitmapOffset = part->blockBitmap.bits + offsetSize;
            break;
        default:
            break;
    }
    ideWrite(part->disk, sectorLBA, bitmapOffset, 1);
}
// 创建文件，成功则返回文件描述符，否则返回-1
int32 fileCreate(struct Dir* parentDir, char* fileName, uint8 flag){
    // 分配公共缓冲区
    void* ioBuffer = sys_malloc(1024);
    if(ioBuffer == NULL){
        printk("FS_FILE_CREATE: sys_malloc for ioBuffer failed!\n");
        return -1;
    }
    uint8 rollbackStep = 0;
    int32 inodeNum = inodeBitmapAlloc(currentPart);
    if(inodeNum == -1){
        printk("FS_FILE_CREATE: alloc inode failed!\n");
        return -1;
    }
    struct Inode* newFileInode = (struct Inode*)sys_malloc(sizeof(struct Inode));
    if(newFileInode == NULL){
        printk("FS_FILE_CREATE: sys_malloc for inode failed!\n");
        rollbackStep = 1;
        goto rollback;
    }
    inodeInit(inodeNum, newFileInode);
    int fdIndex = getFileTableIndex();
    if(fdIndex == -1){
        printk("FS_FILE_CREATE: exceed max files\n");
        rollbackStep = 2;
        goto rollback;
    }
    fileTable[fdIndex].fdInode  = newFileInode;
    fileTable[fdIndex].fdPos = 0;
    fileTable[fdIndex].fdFlag = flag;
    fileTable[fdIndex].fdInode->writeDeny = false;
    struct DirEntry newDirEntry;
    memset(&newDirEntry, 0, sizeof(struct DirEntry));
    createDirEntry(fileName, inodeNum, FT_FILE, &newDirEntry);
    /* 同步内存数据到硬盘
     *
     */
    if(!syncDirEntry(parentDir, &newDirEntry, ioBuffer)){
        printk("FS_FILE_CREATE: sync dir entry to disk failed\n");
        rollbackStep = 3;
        goto rollback;
    }
    // 将父结点的inode同步至硬盘
    memset(ioBuffer, 0, 1024);
    inodeSync(currentPart, parentDir->inode, ioBuffer);
    // 将新创建的文件inode同步到硬盘
    memset(ioBuffer, 0, 1024);
    inodeSync(currentPart, newFileInode, ioBuffer);
    // 将inodeBitmap位图同步到硬盘
    bitmapSync(currentPart, inodeNum, INODE_BITMAP);
    // 将创建的文件inode添加到openInodes链表
    listPush(&currentPart->openInodes, &newFileInode->inodeTag);
    newFileInode->opentCnt = 1;
    sys_free(ioBuffer);
    return installFdInPCB(fdIndex);
rollback:
    switch (rollbackStep)
    {
        case 3:
            memset(&fileTable[fdIndex], 0, sizeof(struct File));
        case 2:
            sys_free(newFileInode);
        case 1:
            setBitmap(&currentPart->inodeBitmap, inodeNum, 0);
        break;
    }
    sys_free(ioBuffer);
    return -1;
}
// 打开编号为inodeNum的inode对应的文件，成功则返回文件描述符，失败则返回-1
int32 fileOpen(uint32 inodeNum, uint8 flag){
    int fdIndex = getFileTableIndex();
    if(fdIndex == -1){
        printk("exceed max open files\n");
        return -1;
    }
    fileTable[fdIndex].fdInode = inodeOpen(currentPart, inodeNum);
    // 每次打开都需要重新置0
    fileTable[fdIndex].fdPos = 0;
    fileTable[fdIndex].fdFlag = flag;
    bool* writeDeny = &fileTable[fdIndex].fdInode->writeDeny;
    if(flag & O_WRONLY || flag & O_RDWR){
        //只要是写文件，判断其他线程是否在写此文件，如果是读文件则不需要考虑
        // 进入临界区时先关中断
        enum IntrStatus oldStatus = intrDisable();
        if(!(*writeDeny)){
            // 如果没有其他线程在写，标记为写
            *writeDeny = true;
            setIntrStatus(oldStatus);
        }else{
            // 直接返回失败
            setIntrStatus(oldStatus);
            printk("FS_FILE_OPEN: File can't be write now, try again later!\n");
            return -1;
        }
    }
    return installFdInPCB(fdIndex);
}
// 把buffer中count个字节写入file, 成功返回写入的字节数，失败返回-1
int32 fileWrite(struct File* file, const void* buffer, uint32 count){
    if((file->fdInode->dataSize + count) > (BLOCK_SIZE * 140)){
        // 文件超过可以读写的最大长度: 512 * 140 = 71680 Bytes
        printk("FS_FILE_WRITE: exced max file size 71680 bytes, write file failed!\n");
        return -1;
    }
    uint8* ioBuff = sys_malloc(BLOCK_SIZE);
    if(ioBuff == NULL){
        printk("FS_FILE_WRITE: alloc io buffer failed!\n");
        return -1;
    }
    uint32* allBlocks = (uint32*)sys_malloc(BLOCK_SIZE + 48);
    if(allBlocks == NULL){
        printk("FS_FILE_WRITE: alloc all blocks failed!\n");
        return -1;
    }
    struct BlockInfo blockInfo = {-1,0};
    // 指向待写入的数据
    const uint8* src = buffer;
    // 已经写入的字节数
    uint32 writtenBytesCnt = 0;
    // 剩余待写的字节数
    uint32 leftBytesCnt = count;
    // blockBitmap位索引
    uint32 sectorIndex; // 扇区索引
    uint32 sectorLBA; // 扇区LBA地址
    uint32 chunkSize = 0;       // 每次写如硬盘的数据大小
    uint32 blockIndex;  // 块索引
    // 计算写入count个字节前，该文件占用的块数
    uint32 usedBlocksCnt = file->fdInode->dataSize == 0 ? 0 : file->fdInode->dataSize / BLOCK_SIZE + 1;
    // count个字节将使用的块数
    uint32 needBlockCnt = count == 0 ? count : count / BLOCK_SIZE + 1;
    // 计算是否需要申请新的块, 如果增量为0表示块够使用
    uint32 addBLockCnt = needBlockCnt - usedBlocksCnt;
    // 1、根据addBlockCnt新增blocK
    uint32 index = 0;
    uint32 addTabel[12 +128] = {0};
    while(index < addBLockCnt){
        // 申请一个块
        blockInfo.blockIndex = -1;
        blockInfo.blockLBA = blockAlloc(currentPart);
        if(blockInfo.blockLBA == -1){
           goto FS_FO_ROLLBACK;
        }
        if(!inodeAddBlock(currentPart, file->fdInode, &blockInfo)){
            goto FS_FO_ROLLBACK;
        }
        index++;
    }
    // 2、读取可用的blockLBA至allBlocks
    blockIndex = 0;
    while (blockIndex < 12)
    {
        allBlocks[blockIndex] = file->fdInode->blockTable[blockIndex];
        blockIndex++;
    }
    inodeReadL1BlocksLBA(currentPart, file->fdInode, allBlocks + 12);
    // 3、写数据
    sectorIndex = 0;
    file->fdInode->dataSize = 0;
    while(writtenBytesCnt < count){
        memset(ioBuff, 0, BLOCK_SIZE);
        while(allBlocks[sectorIndex] == 0){
            sectorIndex++;
        }
        ASSERT(sectorIndex < 140);
        chunkSize = leftBytesCnt < SECTOR_SIZE ? leftBytesCnt : SECTOR_SIZE;
        sectorLBA = allBlocks[sectorIndex];
        memcpy(ioBuff, src, chunkSize);
        ideWrite(currentPart->disk, sectorLBA, ioBuff, BLOCK_SIZE/SECTOR_SIZE);
        printkf("File write at LBA: 0x%04x\n", sectorLBA);
        src += chunkSize;
        file->fdInode->dataSize += chunkSize;
        writtenBytesCnt += chunkSize;
        leftBytesCnt -= chunkSize;
        sectorIndex++;
    }
    inodeSync(currentPart, file->fdInode, ioBuff);
    sys_free(allBlocks);
    sys_free(ioBuff);
    return writtenBytesCnt;
    // 回滚操作
FS_FO_ROLLBACK:
    // 撤销分配的块
    index = 0;
    while(index < 140){
        if(addTabel[index] != 0){
            blockInfo.blockIndex = -1;
            blockInfo.blockLBA = addTabel[index];
            inodeRemoveBlock(currentPart, file->fdInode, &blockInfo);
            blockRelease(currentPart, addTabel[index]);
        }
        index++;
    }
    return -1;
}
// 从file中读取count个字节写入buffer, 成功返回读取到的字节数，失败返回-1
int32 fileRead(struct File* file, void* buffer, uint32 count){
    printkf("FileInfo: INode=%d, BlockInfo:\n",file->fdInode->inodeNum);
    int32 index = 0;
    while(index < 13){
        printkf("    index=%d, LBA=%d\n", index, file->fdInode->blockTable[index]);
        index++;
    }
    uint8* destBuff = buffer;
    uint32 size = count;
    uint32 leftSize = count;
    // 判断读取的数据大小是否比文件长
    if((file->fdPos + count) > file->fdInode->dataSize){
        size = file->fdInode->dataSize - file->fdPos;
        if(size == 0){
            return -1;
        }
        leftSize = size;
    }
    uint8* ioBuf = (uint8*)sys_malloc(BLOCK_SIZE);
    if(ioBuf == NULL){
        printk("FS_READ_FILE: malloc io buffer failed!\n");
        return -1;
    }
    uint32* allBlocks = (uint32*)sys_malloc(140 * 4);
    printkf("AB VADDR:0x%8x\n", (uint32)allBlocks);
    if(allBlocks == NULL){
        printk("FS_READ_FILE: malloc block LBA tabel failed!\n");
        return -1;
    }
    // 2、读取所有的block
    int32 blockIndex = 0;
    inodeReadBlocks(currentPart, file->fdInode, allBlocks);
    while(blockIndex < 140){
        if(allBlocks[blockIndex] != 0){
            printkf("TIDX=%d, LBA=%d\n", blockIndex ,allBlocks[blockIndex]);
        }
        blockIndex++;
    }
    blockIndex = 0;
    // 3、读取文件
    //uint32 readBlockCnt = readEndIndex - readStartIndex;
    uint32 chunkSize = 0;
    uint32 secOffsetCnt = 0;
    uint32 secLeftCnt = 0;
    uint32 bytesRead = 0;
    uint32 sectorLBA = 0;
    while(bytesRead < size){
        blockIndex = file->fdPos / BLOCK_SIZE;
        printkf("LBA=%d\n", allBlocks[blockIndex]);
        // 提示：出现为0时，可能需要重新设计FS
        ASSERT(allBlocks[blockIndex] != 0);
        sectorLBA = allBlocks[blockIndex];
        secOffsetCnt = file->fdPos % BLOCK_SIZE;
        secLeftCnt = BLOCK_SIZE - secOffsetCnt;
        chunkSize = leftSize < secLeftCnt ? leftSize : secLeftCnt;
        memset(ioBuf, 0, BLOCK_SIZE);
        ideRead(currentPart->disk, sectorLBA, ioBuf, BLOCK_SIZE / SECTOR_SIZE);
        memcpy(destBuff, ioBuf + secOffsetCnt, chunkSize);
        destBuff += chunkSize;
        file->fdPos += chunkSize;
        bytesRead += chunkSize;
        leftSize -= chunkSize;
    }
    sys_free(allBlocks);
    sys_free(ioBuf);
    return bytesRead;
}
// 关闭文件, 成功关闭返回0，失败返回-1
int32 fileClose(struct File* file){
    if(file == NULL) return -1;
    file->fdInode->writeDeny = false;
    inodeClose(file->fdInode);
    file->fdInode = NULL;
    return 0;
}