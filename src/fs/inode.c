#include "inode.h"
#include "ide.h"
#include "debug.h"
#include "string.h"
#include "list.h"
#include "thread.h"
#include "constant.h"
#include "interrupt.h"
#include "file.h"
// 根据blockIndex分析，该块类型（直接块，间接块）
/*
static enum BlockType analyseBlockType(int blockIndex){
    if(blockIndex < 12){
        return L0_BLOCK;
    }else if(blockIndex >= 12 && blockIndex < (12 + 128)){
        return L1_BLOCK;
    }else if(blockIndex >= (12 + 128) && blockIndex < (12 + 128 + 128 * 128)){
        return L2_BLOCK;
    }else{
        return L3_BLOCK;
    }
}*/
// 根据inode号获取inode位置信息
static void inodeLocate(struct Partition* part, uint32 inodeNum, struct InodePosition* inodePos){
    /* inodeTable在硬盘山是连续的*/
    ASSERT(inodeNum < 4096);
    uint32 inodeTableLBA = part->superBlock->inodeTableLba;
    uint32 inodeSize = sizeof(struct Inode);
    uint32 offset = inodeNum * inodeSize;
    uint32 offsetSectorCnt = offset / 512;
    uint32 offsetInSector = offset % 512;

    // 判断inode是否跨扇区
    uint32 leftSize = 512 - offsetInSector;
    if(leftSize < inodeSize){
        // 剩下的空间不足以容纳inode,跨扇区
        inodePos->twoSector = true;
    }else{
        inodePos->twoSector = false;
    }
    inodePos->sectorLBA = inodeTableLBA + offsetSectorCnt;
    inodePos->offset = offsetInSector;
}
// 把inode写入分区part
void inodeSync(struct Partition* part, struct Inode* inode, void* buffer){
    uint8 inodeNum = inode->inodeNum;
    struct InodePosition inodePos;
    inodeLocate(part, inodeNum, &inodePos);
    ASSERT(inodePos.sectorLBA <= (part->startLBA + part->sectorCount));
    /* inode成员中的inodeTag、openCnt和writeDeny不需要写入硬盘，写时全部设置为0
     */
    struct Inode pureInode;
    memcpy(&pureInode, inode, sizeof(struct Inode));
    pureInode.opentCnt = 0;
    pureInode.writeDeny = false;
    pureInode.inodeTag.prev = NULL;
    pureInode.inodeTag.next = NULL;
    char* inodeBuffer = (char*)buffer;
    // 先读取inode所在的扇区，使用新的inode替换旧的inode后再写回, 跨区域时需要读取两个扇区
    uint32 sectorCnt = inodePos.twoSector == true ? 2 : 1;
    // 跨两个扇区时，需要读取两个扇区出来
    ideRead(part->disk, inodePos.sectorLBA, inodeBuffer, sectorCnt);
    // 替换旧的inode
    memcpy((inodeBuffer + inodePos.offset), &pureInode, sizeof(struct Inode));
    // 写入
    ideWrite(part->disk, inodePos.sectorLBA, inodeBuffer, sectorCnt);
}
// 将硬盘上的inode清空
void inodeDelete(struct Partition* part, uint32 inodeNum, void* ioBuff){
    ASSERT(inodeNum < 4096);
    struct InodePosition inodePos;
    inodeLocate(part, inodeNum, &inodePos);
    //printkf("IPOS: SecLBA=%d, Offset=%d, TwoSec=%d\n", inodePos.sectorLBA, inodePos.offset, inodePos.twoSector);
    char* inodeBuf = (char*)ioBuff;
    uint32 sectorCnt = inodePos.twoSector == true ? 2 : 1;
    ideRead(part->disk, inodePos.sectorLBA, inodeBuf, sectorCnt);
    memset(inodeBuf + inodePos.offset, 0, sizeof(struct Inode));
    ideWrite(part->disk, inodePos.sectorLBA, inodeBuf, sectorCnt);
}
// 根据inode号返回inode信息
struct Inode* inodeOpen(struct Partition* part, uint32 inodeNum){
    // 在已打开的inode查找该inode是否已近被打开
    struct ListElem* elem = part->openInodes.head.next;
    struct Inode* inode;
    while(elem != &part->openInodes.tail){
        inode = elem2entry(struct Inode, inodeTag, elem);
        if(inode->inodeNum == inodeNum){
            inode->opentCnt++;
            return inode;
        }
        //printk("Find inode in opent list\n");
        elem = elem->next;
    }
    /* 找不到时会执行到这里
     * 打开的inode会被所有任务共享，需要把inode的信息保存在内核空间
     * 根据memor的申请规则，需要临时将线程的pageDir置为NULL
     */
    struct InodePosition inodePos;
    inodeLocate(part, inodeNum, &inodePos);
    //printkf("InodePos: inodeNum=0x%08x, LBA=%d, offset=%d\n", inodeNum, inodePos.sectorLBA, inodePos.offset);
    // 备份pageDir
    struct TaskStruct* currentTask = runningThread();
    uint32* pageDir = currentTask->pageDir;
    currentTask->pageDir = NULL;
    inode = (struct Inode*)sys_malloc(sizeof(struct Inode));
    memset(inode, 0, sizeof(struct Inode));
    currentTask->pageDir =pageDir;  //  恢复pageDir
    // 读取inode, 跨扇区时需要读取两个扇区
    uint32 sectorCnt = inodePos.twoSector == true ? 2 : 1;
    char* inodeBuffer = (char*)sys_malloc(sectorCnt * 512);
    ideRead(part->disk, inodePos.sectorLBA, inodeBuffer, sectorCnt);
    memcpy(inode, inodeBuffer + inodePos.offset, sizeof(struct Inode));
    // 加入打开的inode队列，加入到队首便于快速检索
    listPush(&part->openInodes, &inode->inodeTag);
    inode->opentCnt = 1;
    sys_free(inodeBuffer);
    // debug
    //printkf("OPEN_INODE: NUM=%0x8x, Table[12]=%d\n", inode->inodeNum, inode->blockTable[12]);
    return inode;
}

// 关闭或减少inode的打开次数
void inodeClose(struct Inode* inode){
    // 如果没有进程打开此文件，将inode从全局中移除并释放空间
    enum IntrStatus oldStatus = intrDisable();
    inode->opentCnt--;
    if(inode->opentCnt == 0){
        listRemove(&inode->inodeTag);
        struct TaskStruct* currentTask = runningThread();
        uint32* pageDir = currentTask->pageDir;
        currentTask->pageDir = NULL;
        sys_free(inode);
        currentTask->pageDir = pageDir;
    }
    setIntrStatus(oldStatus);
}
// 回收inode本身和inode对应的数据块
void inodeRelease(struct Partition* part, uint32 inodeNum){
    uint32 allBlocks[140] = {0};
    struct Inode* delInode = inodeOpen(part, inodeNum);
    inodeReadBlocks(part,delInode, allBlocks);
    uint32 index = 0;
    while(index < 140){
        if(allBlocks[index] != 0){
            blockRelease(part, allBlocks[index]);
        }
        index++;
    }
    // 回收inode占用的Inode号
    setBitmap(&part->inodeBitmap, inodeNum, 0);
    bitmapSync(part, inodeNum, INODE_BITMAP);
    // ToDo: 调试使用，清除inode对应的数据，便于理解
    void* ioBuff = sys_malloc(1024);
    inodeDelete(part, inodeNum, ioBuff);
    sys_free(ioBuff);
    // 调试用结束
    inodeClose(delInode);
}
// 初始化inode
void inodeInit(uint32 inodeNum, struct Inode* inode){
    inode->inodeNum = inodeNum;
    inode->dataSize = 0;
    inode->opentCnt = 0;
    inode->creatTime = 0;
    inode->modifyTime = 0;
    inode->writeDeny = false;
    /* 初始化块索引数组
     * 目前打算实现到二级索引块即 blockIndex = 13
     */
    uint8 blockIndex = 0;
    while(blockIndex < BLOCK_TABLE_LEN){
        inode->blockTable[blockIndex] = 0;
        blockIndex++;
    }
}
// 在inode的直接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL0Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    int32 blockTableIndex = 0;
    if(blockInfo->blockIndex != -1){
        inode->blockTable[blockInfo->blockIndex] = blockInfo->blockLBA;
        return true;
    }
    while(blockTableIndex < 12){
        if(inode->blockTable[blockTableIndex] == 0){
            inode->blockTable[blockTableIndex] = blockInfo->blockLBA;
            blockInfo->blockIndex = blockTableIndex;
            return true;
        }
        blockTableIndex++;
    }
    return false;
}
bool inodeRemoveL0Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    // 根据blockIndex，验证后直接删除
    if(blockInfo->blockIndex != -1){
        if(inode->blockTable[blockInfo->blockIndex] == blockInfo->blockLBA){
            inode->blockTable[blockInfo->blockIndex] = 0;
            return true;
        }else{
            return false;
        }
    }
    // 遍历查找
    int32 blockIndex = 0;
    while(blockIndex < 12){
        if(inode->blockTable[blockIndex] == blockInfo->blockLBA){
            inode->blockTable[blockIndex] = 0;
        }
    }
    return false;
}
// 在inode的L1间接接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL1Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    uint32* tableL1 = sys_malloc(BLOCK_SIZE);
    int32 blockTableLBA = -1;
    if(tableL1 == NULL){
        printk("FS_ADD_L1_BLOCK: alloc tableL1 failed!\n");
        return false;
    }
    if(inode->blockTable[12] == 0){
        // 第一次使用， 申请一个新的块作为索引表
        blockTableLBA = blockAlloc(part);
        if(blockTableLBA == -1){
            printk("FS_ADD_L1_BLOCK: alloc block failed!\n");
            sys_free(tableL1);
            return false;
        }
        inode->blockTable[12] = blockTableLBA;
        ideRead(part->disk, blockTableLBA, tableL1, BLOCK_SIZE / SECTOR_SIZE);
        // 可能有脏数据，需要清0
        memset(tableL1, 0, BLOCK_SIZE);
    }else{
        blockTableLBA = inode->blockTable[12];
        ideRead(part->disk, blockTableLBA, tableL1, BLOCK_SIZE / SECTOR_SIZE);
    }
    int32 tabelIndex = 0;
    while (tabelIndex < 128)
    {
        if(tableL1[tabelIndex] == 0){
            tableL1[tabelIndex] = blockInfo->blockLBA;
            // 同步数据到硬盘
            ideWrite(part->disk, blockTableLBA, tableL1, BLOCK_SIZE / SECTOR_SIZE);
            sys_free(tableL1);
            return true;
        }
        tabelIndex++;
    }
    sys_free(tableL1);
    // 运行到这里时表示L1索引块已经没有空位，此时，一级间接块已经存在，不需要回滚操作，直接返回false
    return false;
}
// 把L1对应的索引表读取到ioBuff中
void inodeReadL1BlocksLBA(struct Partition* part, struct Inode* inode, void* ioBuff){
    printkf("IRL1B: IOB VADDR:0x%8x\n", ioBuff);
    if(inode->blockTable[12] == 0) return;
    ideRead(part->disk, inode->blockTable[12], ioBuff, BLOCK_SIZE / SECTOR_SIZE);
}
bool inodeRemoveL1Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    if(inode->blockTable[12] == 0){
        printk("FS_REMOVE_L1_BLOCK: inode no L1 block!\n");
        return false;
    }
    // 读取一级索引表
    uint32* indexTabel = (uint32*)sys_malloc(BLOCK_SIZE);
    if(indexTabel == NULL){
        printk("FS_REMOVE_L1_BLOCK: failed alloc index tabel\n");
        return false;
    }
    ideRead(part->disk, inode->blockTable[12], indexTabel, BLOCK_SIZE / SECTOR_SIZE);
    uint32 tabelIndex = 0;
    uint32 blockIndex = 0;
    //uint32 blockLBA = 0;
    if(blockInfo->blockIndex != -1){
        blockIndex = blockInfo->blockIndex;
        tabelIndex = blockIndex - 12;
        if(indexTabel[tabelIndex] == blockInfo->blockLBA){
            blockRelease(part, blockInfo->blockLBA);
            indexTabel[tabelIndex] = 0;
            ideWrite(part->disk, inode->blockTable[12], indexTabel, BLOCK_SIZE / SECTOR_SIZE);
            sys_free(indexTabel);
            return true;
        }
        sys_free(indexTabel);
        return false;
    }else{
        // 遍历查找
        tabelIndex = 0;
        while(tabelIndex < 128){
            if(indexTabel[tabelIndex] == blockInfo->blockLBA){
                blockRelease(part, blockInfo->blockLBA);
                indexTabel[tabelIndex] = 0;
                ideWrite(part->disk, inode->blockTable[12], indexTabel, BLOCK_SIZE / SECTOR_SIZE);
                sys_free(indexTabel);
                return true;
            }
            tabelIndex++;
        }
        // 遍历完后没有找到，移除失败
        sys_free((void*)indexTabel);
        return false;
    }
}

// 在inode的L2间接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL2Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo)
{
    // 目前不支持， 返回false
    return false;
}
bool inodeRemoveL2Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    // 目前不支持， 返回false
    return false;
}

// 在inode的L3间接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL3Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    // 目前不支持， 返回false
    return false;
}
bool inodeRemoveL3Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo)
{
    // 目前不支持， 返回false
    return false;
}
bool inodeAddBlock(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    if(inodeAddL0Block(part, inode, blockInfo)){
        return true;
    }else if(inodeAddL1Block(part, inode, blockInfo)){
        return true;
    }else if(inodeAddL2Block(part, inode, blockInfo)){
        return true;
    }else if(inodeAddL3Block(part, inode, blockInfo)){
        return true;
    }
    return false;
}
void inodeReadBlocks(struct Partition* part, struct Inode* inode, uint32* blockTableBuff){
    uint32 tabeleIndex = 0;
    while(tabeleIndex < 12){
        blockTableBuff[tabeleIndex] = inode->blockTable[tabeleIndex];
        tabeleIndex++;
    }
    if(inode->blockTable[12] == 0) return;
    ideRead(part->disk, inode->blockTable[12], blockTableBuff + 12, BLOCK_SIZE / SECTOR_SIZE);
    //inodeReadL1BlocksLBA(part, inode, blockTableBuff + 12);
    /*if(inode->blockTable[13] == 0) return;
    inodeReadL2BlocksLBA(part, inode, blockTableBuff + 12 + 128);
    if(inode->blockTable[14] == 0) return;
    inodeReadL3BlocksLBA(part, inode, blockTableBuff + 12 + 128 + 128 * 128);*/
}
bool inodeRemoveBlock(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo){
    if(inodeRemoveL0Block(part, inode, blockInfo)){
        return true;
    }else if(inodeRemoveL1Block(part, inode, blockInfo)){
        return true;
    }else if(inodeRemoveL2Block(part, inode, blockInfo)){
        return true;
    }else if(inodeRemoveL3Block(part, inode, blockInfo)){
        return true;
    }
    return false;
}