#include "dir.h"
#include "stdint.h"
#include "ide.h"
#include "inode.h"
#include "memory.h"
#include "constant.h"
#include "printk.h"
#include "string.h"
#include "file.h"
#include "debug.h"
#include "block.h"
// 根目录
struct Dir rootDir;

// 打开根目录
void openRootDir(struct Partition* part){
    rootDir.inode = inodeOpen(part, part->superBlock->rootInodeNum);
    rootDir.dirPos = 0;
}

// 在分区part上打开inode位inodeNum的目录并返回目录指针
struct Dir* dirOpen(struct Partition* part, uint32 inodeNum){
    struct Dir* dir = (struct Dir*)sys_malloc(sizeof(struct Dir));
    dir->inode = inodeOpen(part, inodeNum);
    dir->dirPos = 0;
    return dir;
}

/* 在分区part的dir目录中寻找名为name的文件或目录
 * 找到返回true，并把目录项写入到dirEntry中，否则返回false
 */
bool searchDirEntry(struct Partition* part, struct Dir* dir, const char* name, struct DirEntry* dirEntry){
    // block的数量，这个操作系统中，支持1级间接索引块
    uint32 blockCnt = 140; // 12 +128
    // 分配空间，用来保存文件的块inodeNum
    uint32* allBlocks = (uint32*)sys_malloc(blockCnt * 4);
    if(allBlocks == NULL){
        printk("SEARCH_DIR_ENTRY: malloc allblocks failed\n");
        return false;
    }
    uint32 blockIndex = 0;
    while (blockIndex < BLOCK_TABLE_LEN)
    {
        allBlocks[blockIndex] = dir->inode->blockTable[blockIndex];
        blockIndex++;
    }
    if(dir->inode->blockTable[12] != 0){
        //printkf("RB: BlockLBA = %2d\n", dir->inode->blockTable[12]);
        ideRead(currentPart->disk, dir->inode->blockTable[12], allBlocks + 12, 1);
    }
    // 写目录时已经保证目录项不跨扇区，只需申请一个扇区的内存作为缓冲区
    uint8* buffer = (uint8*)sys_malloc(SECTOR_SIZE);
    struct DirEntry* dirEntryPtr = (struct DirEntry*)buffer;
    uint32 dirEntrySize = part->superBlock->dirEntrySize;
    uint32 dirEntryCnt = SECTOR_SIZE / dirEntrySize;
    // 开始在所有块中查找目录项
    blockIndex = 0;
    while(blockIndex < blockCnt){
        //printkf("BLOCK_INDEX=%d\n", blockIndex);
        // 地址为0时该数据块中没有数据，继续在其他块中查找
        if(allBlocks[blockIndex] == 0){
            blockIndex++;
            continue;
        }
        // 有数据时读取对应扇区
        //printkf("    Find data: BI=%d, BLAB=%d\n", blockIndex, allBlocks[blockIndex]);
        ideRead(part->disk, allBlocks[blockIndex], buffer, 1);
        uint32 dirEntryIndex = 0;
        while(dirEntryIndex < dirEntryCnt){
            // 如果找到，直接复制整个目录项,相等时strcmp返回0
            if(!strcmp(dirEntryPtr->fileName, name)){
                memcpy(dirEntry, dirEntryPtr, dirEntrySize);
                sys_free(buffer);
                sys_free(allBlocks);
                return true;
            }
            dirEntryIndex++;
            dirEntryPtr++;
        }
        blockIndex++;
        // 此时，dirEntryPtr已近指向buffer的最后，需要重新初始化
        dirEntryPtr = (struct DirEntry*)buffer;
        memset(buffer, 0, SECTOR_SIZE);
    }
    sys_free(buffer);
    sys_free(allBlocks);
    return false;
}

/* 关闭目录 */
void dirClose(struct Dir* dir){
    // 根目录不能关闭，而且根目录的位置不能释放
    if(dir == &rootDir) return;
    inodeClose(dir->inode);
    sys_free(dir);
}
/* 在内存中初始化目录项 */
void createDirEntry(char* fileName, uint32 inodeNum, uint8 fileType, struct DirEntry* dirEntry){
    ASSERT(strlen(fileName) <= MAX_FILE_NAME_LEN);
    // 初始化数据
    memcpy(dirEntry->fileName, fileName, strlen(fileName));
    dirEntry->inodeNum = inodeNum;
    dirEntry->fileType = fileType;
}

/* 将目录项dirEntry写入父目录parentDir中， ioBuffer由主调函数提供 */
bool syncDirEntry(struct Dir* parentDir, struct DirEntry* dirEntry, void* ioBuffer){
    struct BlockInfo blockInfo = {-1, 0};
    struct Inode* pInode = parentDir->inode;
    uint32 dirDataSize = pInode->dataSize;
    uint32 dirEntrySize = currentPart->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_SIZE / dirEntrySize;
    int32 blockLBA = -1;
    uint32 blockIndex = 0;
    uint32 blockCnt = 140;
    uint32 allBlocks[140];
    // 读取复制blockTable
    while(blockIndex < BLOCK_TABLE_LEN){
        allBlocks[blockIndex] = pInode->blockTable[blockIndex];
        blockIndex++;
    }
    // 开始遍历所有块以查找，空位
    struct DirEntry* dirEntryPtr = (struct DirEntry*)ioBuffer;
    //int32 blockBitmapIndex = -1;
    blockIndex = 0;
    while(blockIndex < blockCnt){
        //blockBitmapIndex = -1;
        if(allBlocks[blockIndex] == 0){
            blockLBA = blockAlloc(currentPart);
            if(blockLBA == -1){
                printk("SYNC_DIR_ENTRY: alloc block failed!\n");
                return false;
            }
            blockInfo.blockIndex = blockIndex;
            blockInfo.blockLBA = blockLBA;
            if(blockIndex < 12){
                inodeAddL0Block(currentPart, pInode, &blockInfo);
                //pInode->blockTable[blockIndex] = blockLBA;
                allBlocks[blockIndex] = blockLBA;
            }else if(blockIndex == 12){
                if(!inodeAddL1Block(currentPart, pInode, &blockInfo)){
                    blockRelease(currentPart, blockInfo.blockLBA);
                    printk("SYNC_DIR_ENTRY: add inode L1 block failed!\n");
                    return false;
                }
                allBlocks[12] = blockLBA;
                // 把新分配的第0个间接块写入一级间接块表
                ideWrite(currentPart->disk, pInode->blockTable[12], allBlocks + 12, 1);
            }else{
                // 这部分是13~140部分，程序运行到此处意味着访问间接块
                allBlocks[blockIndex] = blockLBA;
                // 在此写入一级间接块索引表
                ideWrite(currentPart->disk, pInode->blockTable[12], allBlocks + 12, 1);
            }
            /* 写入第一个dirEntry */
            memset(ioBuffer, 0, 512);
            memcpy(ioBuffer, dirEntry, dirEntrySize);
            ideWrite(currentPart->disk, allBlocks[blockIndex], ioBuffer, 1);
            pInode->dataSize += dirEntrySize;
            return true;
        }
        ideRead(currentPart->disk, allBlocks[blockIndex], ioBuffer, 1);
        uint32 dirEntryIndex = 0;
        while(dirEntryIndex < dirEntryCntPerSec){
            if((dirEntryPtr + dirEntryIndex)->fileType == FT_UNKNOWN){
                memcpy(dirEntryPtr + dirEntryIndex, dirEntry, dirEntrySize);
                ideWrite(currentPart->disk, allBlocks[blockIndex], ioBuffer, 1);
                pInode->dataSize += dirEntrySize;
                return true;
            }
            dirEntryIndex++;
        }
        blockIndex++;
    }
    printk("directoty is full!\n");
    return false;
}
// 读取目录，成功返回目录项，失败返回NULL
struct DirEntry* readDirEntry(struct Dir* dir){
    struct DirEntry* dirEntryPtr = (struct DirEntry*)dir->dirBuffer;
    struct Inode* dirInode = dir->inode;
    uint32 allBlocks[140] = {0};
    inodeReadBlocks(currentPart, dirInode, allBlocks);
    uint32 blockIndex = 0;
    uint32 dirEntryIndex = 0;
    uint32 currentDirEntryPos = 0;
    uint32 dirEntrySize = currentPart->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_SIZE / dirEntrySize;
    // 遍历扇区内的目录项
    while(dir->dirPos < dir->inode->dataSize){
        if(dir->dirPos >= dir->inode->dataSize){
            return NULL;
        }
        if(allBlocks[blockIndex] == 0){
            blockIndex++;
            continue;
        }
        memset(dirEntryPtr, 0, SECTOR_SIZE);
        //printkf("IDX:%d,LBA: %d\n", blockIndex, allBlocks[blockIndex]);
        ideRead(currentPart->disk, allBlocks[blockIndex], dirEntryPtr,1);
        // 遍历扇区内的目录项
        dirEntryIndex = 0;
        while(dirEntryIndex < dirEntryCntPerSec){
            // 目录不等于0即TF_UNKONW，判断是不是最新的目录项，避免返回已经返回过的目录项
            if((dirEntryPtr + dirEntryIndex)->fileType){
                if(currentDirEntryPos < dir->dirPos){
                    currentDirEntryPos += dirEntrySize;
                    dirEntryIndex++;
                    continue;
                }
                dir->dirPos += dirEntrySize;
                return dirEntryPtr + dirEntryIndex;
            }
            dirEntryIndex++;
        }
        blockIndex++;
    }
    return NULL;
}
// 删除part父目录为parentDir编号为inodeNum的目录项
bool deleteDirEntry(struct Partition* part, struct Dir* parentDir, uint32 inodeNum, void* ioBuff){
    struct Inode* pinode = parentDir->inode;
    uint32 tableIndex = 0;
    uint32 allBlocks[140] = {0};
    // 1、获取读取所有locks
    inodeReadBlocks(part, pinode,(uint32*)allBlocks);

    // 准备数据
    uint32 dirEntrySize = part->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_SIZE / dirEntrySize;
    struct DirEntry* dirEntryPtr = (struct DirEntry*)ioBuff;
    struct DirEntry* dirEntryFound = NULL;
    uint32 dirEntryIndex = 0;
    uint32 dirEntryCnt = 0;
    bool isDirFirstBlock = false;
    // 遍历目录
    while (tableIndex < 140)
    {
        isDirFirstBlock = false;
        if(allBlocks[tableIndex] == 0){
            tableIndex++;
            continue;
        }
        dirEntryIndex = 0;
        memset(ioBuff, 0, SECTOR_SIZE);
        ideRead(part->disk, allBlocks[tableIndex], ioBuff, 1);
        // 遍历该扇区上的目录项，统计该扇区的目录项数目，以及是否有待删除的目录项
        while(dirEntryIndex < dirEntryCntPerSec){
            if((dirEntryPtr + dirEntryIndex)->fileType == FT_UNKNOWN){
                dirEntryIndex++;
                continue;
            }
            // 提示，相等返回0， 不相等返回1
            if(!strcmp((dirEntryPtr + dirEntryIndex)->fileName, ".")){
                isDirFirstBlock = true;
            }else if(strcmp((dirEntryPtr + dirEntryIndex)->fileName, ".") && strcmp((dirEntryPtr + dirEntryIndex)->fileName, "..")){
                // 统计除了"."、".."之外的目录项信息
                dirEntryCnt++;
                if((dirEntryPtr + dirEntryIndex)->inodeNum == inodeNum){
                    dirEntryFound = dirEntryPtr + dirEntryIndex;
                }
            }
            dirEntryIndex++;
        }
        // 如果没有找到，下一个扇区
        if(dirEntryFound == NULL){
            tableIndex++;
            continue;
        }
        // 如果只余一个目录项，且不是dir的第一个块
        if(dirEntryCnt == 1 && !isDirFirstBlock){
            // 在块位图中回收该块
            blockRelease(part, allBlocks[tableIndex]);
            if(tableIndex < 12){
                pinode->blockTable[tableIndex] = 0;
            }else{
                // 在一级间接块中移除块
                uint32 L1BlockCnt = 0;
                uint32 L1BlockIndex = 12;
                while(L1BlockIndex < 140){
                    if(allBlocks[L1BlockIndex] != 0){
                        L1BlockCnt++;
                    }
                }
                // 如果L1BlockCnt等于1， 则需要移除一级间接块表
                if(L1BlockCnt > 1){
                    // 只需要移除对应的块索引
                    allBlocks[tableIndex] = 0;
                    ideWrite(part->disk, pinode->blockTable[12], allBlocks+12, 1);
                }else{
                    // 释放一级间接块索引表
                    blockRelease(part, pinode->blockTable[12]);
                    pinode->blockTable[12] = 0;
                }
            }
        }else{
            // 清空当个目录项
            memset(dirEntryFound, 0, dirEntrySize);
            ideWrite(part->disk, allBlocks[tableIndex], ioBuff, 1);
        }
        // 更新并同步inode信息到硬盘
        pinode->dataSize -= dirEntrySize;
        inodeSync(part, pinode, ioBuff);
        return true;
    }
    // 运行到这里的时候，需要检查searchFile函数
    return false;
}
// 判断目录是否为空
bool dirIsEmpty(struct Dir* dir){
    struct Inode* inode = dir->inode;
    // 只有"."和".."两个目录项时为空
    return (inode->dataSize == currentPart->superBlock->dirEntrySize * 2);
}
// 在父目录parentDir中删除子目录childDir对应的目录项
int32 dirRemove(struct Dir* parentDir, struct Dir* childDir){
    struct Inode* childInode = childDir->inode;
    // 如果目录为空，只有blockTable[0]不为0，此处检查，出现错误时，需要重新设计inode的块处理函数
    uint32 blockIndex = 1;
    while(blockIndex < BLOCK_TABLE_LEN){
        ASSERT(childInode->blockTable[blockIndex] == 0);
        blockIndex++;
    }
    void* ioBuff = sys_malloc(SECTOR_SIZE * 2);
    if(ioBuff == NULL){
        printk("FS_DIR_REMOVE: malloc io buffer failed!\n");
        return -1;
    }
    // 在父目录中删除子目录对应的目录项
    deleteDirEntry(currentPart, parentDir, childInode->inodeNum, ioBuff);
    // 回收inode对应的资源，包括blockTable中的块，以及inode资源
    inodeRelease(currentPart, childInode->inodeNum);
    sys_free(ioBuff);
    return 0;
}