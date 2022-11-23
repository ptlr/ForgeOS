#include "fs.h"
#include "printk.h"
#include "ide.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"

// 当前分区,挂载文件系统使用
struct Partition* currentPart;
static bool mountPartition(struct ListElem* partElem,int arg){
    char* partName = (char*)arg;
    struct Partition* part = elem2entry(struct Partition, partTag,partElem);
    // 相等时返回0, 如果分区和传入的名字不相同，直接返回
    // ToDo:: 学习过程中，加了if判断以便配合listTraversal使用，返回false使函数继续执行
    if(strcmp(part->name, partName) != 0) return false;
    currentPart = part;
    struct Disk* hd = currentPart->disk;
    // 存储硬盘上读取的超级块
    struct SuperBlock* sbBuffer = (struct SuperBlock*)sys_malloc(SECTOR_SIZE);
    // 在内存中初始化一个区域用于保存超级块
    currentPart->superBlock = (struct SuperBlock*)sys_malloc(sizeof(struct SuperBlock));
    if(currentPart->superBlock == NULL){
        PANIC("FS_MP_01: alloc memory failed!\n");
    }
    // 1、读入超级块
    memset(sbBuffer, 0, SECTOR_SIZE);
    ideRead(hd, currentPart->startLBA + 1, sbBuffer, 1);
    // 复制超级块
    memcpy(currentPart->superBlock, sbBuffer, sizeof(struct SuperBlock));
    // 2、读取块位图至内存中
    currentPart->blockBitmap.bits = (uint8*)sys_malloc(sbBuffer->blockBitmapSectorCnt * SECTOR_SIZE);
    if(currentPart->blockBitmap.bits == NULL){
        PANIC("FS_MP_02:alloc memory failed!\n");
    }
    currentPart->blockBitmap.length = sbBuffer->blockBitmapSectorCnt * SECTOR_SIZE;
    ideRead(hd, sbBuffer->blockBitmapLba, currentPart->blockBitmap.bits, sbBuffer->blockBitmapSectorCnt);

    // 3、读取inode位图至内存
    currentPart->inodeBitmap.bits = (uint8*)sys_malloc(sbBuffer->inodeBitmapSectorCnt * SECTOR_SIZE);
    if(currentPart->inodeBitmap.bits == NULL){
        PANIC("FS_MP_03:alloc memory failed!\n");
    }
    currentPart->inodeBitmap.length = sbBuffer->inodeBitmapSectorCnt * SECTOR_SIZE;
    ideRead(hd, sbBuffer->inodeBitmapLba, currentPart->inodeBitmap.bits, sbBuffer->inodeBitmapSectorCnt);
    listInit(&currentPart->openInodes);
    printkf("    mount %s\n", currentPart->name);
    return true;
}
// 格式化分区
static void partitionFormat(struct Partition* part){
    uint32 bootSectorCnt = 1;
    uint32 superBlockCnt = 1;
    uint32 inodeBitmapSectorCnt = DIV_ROUND_UP(MAX_FILE_NUM_PER_PART, BITS_PER_SECTOR);
    uint32 inodeTableSectorCnt = DIV_ROUND_UP((sizeof(struct Inode) * MAX_FILE_NUM_PER_PART), SECTOR_SIZE);
    uint32 usedSectorCnt = bootSectorCnt + superBlockCnt + inodeBitmapSectorCnt + inodeTableSectorCnt;
    uint32 freeSectorCnt = part->sectorCount - usedSectorCnt;
    // 计算块位图信息
    // Todo: 采用简单计算或者复杂计算
    uint32 blockBitmapSectorCnt = DIV_ROUND_UP(freeSectorCnt,  BITS_PER_SECTOR);
    uint32 blockBitmapLen = freeSectorCnt - blockBitmapSectorCnt;
    blockBitmapSectorCnt = DIV_ROUND_UP(blockBitmapLen, BITS_PER_SECTOR);
    // 初始化超级块
    struct SuperBlock sb;
    sb.magic = 0x19940520;
    sb.sectorCount = part->sectorCount;
    sb.inodeCount = MAX_FILE_NUM_PER_PART;
    sb.partLbaBase = part->startLBA;

    sb.blockBitmapLba = sb.partLbaBase + 2;
    sb.blockBitmapSectorCnt = blockBitmapSectorCnt;

    sb.inodeBitmapLba = sb.blockBitmapLba + sb.blockBitmapSectorCnt;
    sb.inodeBitmapSectorCnt = inodeBitmapSectorCnt;

    sb.inodeTableLba = sb.inodeBitmapLba + sb.inodeBitmapSectorCnt;
    sb.inodeTableSectorCnt = inodeTableSectorCnt;
    
    sb.dataStartLba = sb.inodeTableLba + sb.inodeTableSectorCnt;
    sb.rootInodeNum = 0;
    sb.dirEntrySize = sizeof(struct DirEntry);
    
    // 输出格式化信息
    printkf("Part info:\n    Name:%s\n    Magic:0x%x\n    PartLBABase:%d\n    InodeCnt:%d\n    InodeBitmapLBA:%d\n    InodeBitmapSecCnt:%d\n    InodeTableLBA:%d\n    InodeTableSecCnt:%d\n    DataStartLBA:%d\n",
    part->name, sb.magic, sb.partLbaBase, sb.inodeCount, sb.inodeBitmapLba, sb.inodeBitmapSectorCnt, sb.inodeTableLba, sb.inodeTableSectorCnt, sb.dataStartLba);
    struct Disk* disk = part->disk;
    // 1、写入超级块
    ideWrite(disk, part->startLBA + 1, &sb, 1);
    //printkf("Super Block LBA: %d\n", part->startLBA + 1);

    // 找出最大的的元信息，用其尺寸做缓冲区
    uint32 bufferSize = sb.blockBitmapSectorCnt >= sb.inodeBitmapSectorCnt ? sb.blockBitmapSectorCnt : sb.inodeBitmapSectorCnt;
    bufferSize = (sb.inodeTableSectorCnt >= bufferSize ? sb.inodeTableSectorCnt : bufferSize) * SECTOR_SIZE;
    uint8* buffer = (uint8*)sys_malloc(bufferSize);
    // 初始化块位图
    // 第0个块预留给根目录
    buffer[0] = 0x01;
    uint32 blockBitmapLastByte = blockBitmapLen / 8;
    uint8 blockBitmapLastBit = blockBitmapLen % 8;
    // 计算出未足1扇区时的字节数
    uint32 lastSize = SECTOR_SIZE - (blockBitmapLastByte % SECTOR_SIZE);
    // 将用不到的位置置1表示不能用（已占用，避免后面使用到）
    memset(&buffer[blockBitmapLastByte], 0xFF, lastSize);
    // 将最后1byte的有效位置0。
    uint8 bitIndex = 0;
    while(bitIndex <= blockBitmapLastBit){
        buffer[blockBitmapLastByte] &= ~(1 << bitIndex++);
    }
    // 写入blockbitmap
    ideWrite(disk, sb.blockBitmapLba, buffer, sb.blockBitmapSectorCnt);
    // 3、初始化inode位图并写入硬盘
    memset(buffer, 0, bufferSize);
    buffer[0] = 0x01;   // 第一个inode分配给根目录
    /* 由于inode table共4096个inode,inode位图正好占用512字节，共1个扇区，不需要处理多余的字节
     */
    ideWrite(disk, sb.inodeBitmapLba, buffer, sb.blockBitmapSectorCnt);
    // 4、初始化inode数组并写入到sb.inodeTableLBA中
    memset(buffer, 0, bufferSize);
    struct Inode* inode = (struct Inode*)buffer;
    inode->dataSize = sb.dirEntrySize * 2;  // 目录'.'和'..'
    inode->inodeNum = 0;    // 根目录占inode数组中的index为0
    inode->blockTable[0] = sb.dataStartLba;
    printkf("SB_DSLBA=%d\n", inode->blockTable[0]);
    // 其余的blockPtr无需初始化，原因：memset中初始化为0，此时，剩余的blockPtr都为0
    ideWrite(disk, sb.inodeTableLba, buffer, sb.inodeTableSectorCnt);  
    // 5、将根目录写入到sb.dataStartLBA
    memset(buffer, 0, bufferSize);
    struct DirEntry* partDE = (struct DirEntry*)buffer;
    // 初始化当前目录
    memcpy(partDE->fileName,".",1);
    partDE->inodeNum = 0;
    partDE->fileType = FT_DIR;
    partDE++;
    // 初始化父目录
    memcpy(partDE->fileName, "..", 2);
    partDE->inodeNum = 0;           // 根目录的父目录是根目录自己
    partDE->fileType = FT_DIR;

    ideWrite(disk, sb.dataStartLba, buffer, 1);
    //printkf("    RootDirLBA=%06d\n    %s format done\n", sb.dataStartLba, part->name);
    sys_free(buffer);
}
// 将最上层的路劲解析出来
static char* pathParse(char* path, char* nameStore){
    if(path[0] == '/'){
        /* 如果路径中出现多个'/'，比如'///a/b/c'时，忽略多余的'/' */
        while (*(++path) == '/');
    }
    // 开始解析一般路径
    while(*path != '/' && *path != 0){
        *nameStore++ = *path++;
    }
    if(path[0] == 0) return NULL;
    return path;
}
// 返回路劲深度
int32 pathDepthCnt(char* path){
    ASSERT(path != NULL);
    char* mPath = path;
    char name[MAX_FILE_NAME_LEN];
    uint32 depth = 0;
    mPath = pathParse(mPath, name);
    while(name[0]){
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if(mPath){
            mPath = pathParse(mPath, name);
        }
    }
    return depth;
}
/* 搜索文件pathName, 如果找到返回inode号，否则返回-1 */
int searchFile(const char* pathName, struct PathSearchRecord* searchRecord){
    /* 如果目标是根目录，为避免下面冗长的查找，直接返回已知的根目录信息 */
    if(!strcmp(pathName, "/") || !strcmp(pathName, "/.") || !strcmp(pathName, "/..")){
        searchRecord->parentDir = &rootDir;
        searchRecord->fileType = FT_DIR;
        searchRecord->searchPath[0] = 0;
        return 0;
    }
    uint32 pathLen = strlen(pathName);
    ASSERT(pathName[0] == '/' && pathLen > 1 && pathLen < MAX_PATH_LEN);
    char* subPath = (char*)pathName;
    struct Dir* parentDir = &rootDir;
    struct DirEntry dirEntry;
    // 记录解析出来的各级名称，比如‘/a/b/c’,数组的名称分别是"a","b","c"
    char name[MAX_FILE_NAME_LEN] = {0};
    searchRecord->parentDir = parentDir;
    searchRecord->fileType = FT_UNKNOWN;
    uint32 parentInodeNum = 0;          // 父目录的inode号
    subPath = pathParse(subPath, name);
    while(name[0]){ // 如果第一个结束字符是结束字符，结束循环
        ASSERT(strlen(searchRecord->searchPath) < MAX_PATH_LEN);
        // 记录已存在的父目录
        strcat(searchRecord->searchPath, "/");
        strcat(searchRecord->searchPath, name);
        // 在所给的目录中查找文件
        if(searchDirEntry(currentPart, parentDir, name, &dirEntry)){
            memset(name, 0, MAX_FILE_NAME_LEN);
            // 如果subPath不等于NULL，则继续拆分路径
            if(subPath){
                subPath = pathParse(subPath, name);
            }
            if(FT_DIR == dirEntry.fileType){
                // 如果被打开的是目录
                parentInodeNum = parentDir->inode->inodeNum;
                dirClose(parentDir);
                // 更新父目录
                parentDir = dirOpen(currentPart, dirEntry.inodeNum);
                searchRecord->parentDir = parentDir;
                continue;
            }else if(FT_FILE == dirEntry.fileType){
                searchRecord->fileType = FT_FILE;
                return dirEntry.inodeNum;
            }else{
                // 找不到，返回-1
                return -1;
            }
        }else{
            // 留着parentDir不要关闭
            return -1;
        }
    }
    // 执行到这里的时候，已近遍历完了路径，并且查找的文件或目录只有同名目录存在
    dirClose(searchRecord->parentDir);
    /* 保存被查找的直接父目录 */
    searchRecord->parentDir = dirOpen(currentPart, parentInodeNum);
    searchRecord->fileType = FT_DIR;
    return dirEntry.inodeNum;
}
/* 初始化文件系统
 * 搜索文件系统，没有文件系统时再创建文件系统
 */
void initFileSystem(int (* step)(void)){
    printkf("[%2d] init file system\n", step());
    uint8 channelNum = 0;
    uint8 deviceNum = 0;
    uint8 partIndex = 0;
    struct SuperBlock* sbBuffer = (struct SuperBlock*)sys_malloc(SECTOR_SIZE);
    if(sbBuffer == NULL){
        PANIC("File System: alloc memory failed!!!\n");
    }
    while(channelNum < channelCount){
        deviceNum = 0;
        while(deviceNum < 2){
            if(deviceNum == 0){
                deviceNum++;
                continue;
            }
            struct Disk* disk = &channels[channelNum].devices[deviceNum];
            struct Partition* part = disk->primaryParts;
            while(partIndex < 12){
                // partIndex大于等于4时，后面的都是逻辑分区，分区指针更新为逻辑分区
                if(partIndex == 4){
                    part = disk->logicParts;
                }
                if(part->sectorCount != 0){ // 分区存在
                    memset(sbBuffer, 0, SECTOR_SIZE);
                    ideRead(disk, part->startLBA + 1, sbBuffer, 1);
                    // 根据魔数判断是否存在自己的文件系统，存在则无需初始化，其他值认为不存在，需要重新初化。
                    if(sbBuffer->magic != 0x19940520){
                        printkf("formatting %s's partition %s......\n", disk->name, part->name);
                        partitionFormat(part);
                    }else{
                        printkf("    %s has filesystem\n", part->name); 
                    }       
                }
                partIndex++;
                part++;// 下一个分区
            }
            //下一个硬盘
            deviceNum++;
        }
        // 下一个通道
        channelNum++;
    }
    sys_free(sbBuffer);
    // 挂载需要操作的分区
    char defaultPartName[16] = "sdb1";
    // 尝试挂载分区
    listTraversal(&partitionLists, mountPartition, (int)defaultPartName);
    // 打开根目录
    openRootDir(currentPart);
    // 初始化文件表 
    uint32 fdIndex = 0;
    while(fdIndex < MAX_FILE_OPEN_NUM){
        fileTable[fdIndex++].fdInode =NULL;
    }
}

/* 将文件描述符转化为文件表的下表 */
static uint32 fdLocal2Global(uint32 localFd){
    struct TaskStruct* currentTask = runningThread();
    int32 globalFd = currentTask->fdTable[localFd];
    ASSERT(globalFd >= 0 && globalFd < MAX_FILE_OPEN_NUM);
    return (uint32)globalFd;
}
// 打开或创建文件后，返回文件描述符，否则返回-1
int32 sysOpen(const char* pathName, uint8 flags){
    // 目前只能打开文件，暂时不处理目录
    if(pathName[strlen(pathName) - 1] == '/'){
        printkf("FS_SYS_OPEN: can't open directory '%s'\n",pathName);
        return -1;
    }
    ASSERT(flags <= 7);
    int32 fd = -1;
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    // 记录目录深度，帮组判断中间某个目录不存在的情况
    uint32 pathNameDepth = pathDepthCnt((char*)pathName);
    // 检查文件是否存在
    int inodeNum = searchFile(pathName, &searchRecord);
    bool found = inodeNum != -1 ? true : false;
    if(searchRecord.fileType == FT_DIR){
        printk("FS_SYS_OPEN: can't open directory with open(), use openDir()\n");
        dirClose(searchRecord.parentDir);
        return -1;
    }
    uint32 pathSearchDepth = pathDepthCnt(searchRecord.searchPath);
    // 先判断是否把pathName都访问到了
    if(pathNameDepth != pathSearchDepth){
        // 不相等表示中间某个目录不存在
        printkf("FS_SYS_OPEN: can't access '%s', subpath '%s' is't exist\n",pathName, searchRecord.searchPath);
        dirClose(searchRecord.parentDir);
        return -1;
    }
    // 如果没有找到，并且不是要创建文件，返回-1
    if(!found && !(flags & O_CREAT)){
        printkf("FS_SYS_OPEN: in path '%s',file '%s' not exits\n", searchRecord.parentDir, (strrchr(searchRecord.searchPath, '/') +1));
        dirClose(searchRecord.parentDir);
        return -1;
    } else if(found && flags & O_CREAT){
        printkf("FS_SYS_OPEN: '%s' already exit!\n", pathName);
        dirClose(searchRecord.parentDir);
        return -1;
    }
    switch (flags & O_CREAT)
    {
        case O_CREAT:
            printk("creating file\n");
            fd = fileCreate(searchRecord.parentDir, (strrchr(pathName, '/') + 1), flags);
            dirClose(searchRecord.parentDir);
        break;
        default:
        // 其余情况都是打开文件，O_RDONLY, O_WRONLY, O_RDWR
        fd = fileOpen(inodeNum, flags);
        break;
    }
    return fd;
}
// 将buffer中连续count个字节写如文件描述符fd中，成功返回写入的字节数，失败放回-1
int32 sysWrite(uint32 fd, const void* buffer, uint32 count){
    if(fd < 0){
        printk("FS_SYS_WRITE: fd error!\n");
        return -1;
    }
    if(fd == STD_OUT){
        ASSERT(count < 1024);
        char buff[1024] = {0};
        memcpy(buff, buffer, count);
        consolePrint(buff);
        return count;
    }
    uint32 gfd = fdLocal2Global(fd);
    struct File* writeFile = &fileTable[gfd];
    if(writeFile->fdFlag & O_RDONLY || writeFile->fdFlag & O_RDWR){
        return fileWrite(writeFile, buffer, count);
    }else{
        consolePrint("FS_SYS_WRITE: not allowed to write file without flag O_RDWR or O_WRONLY\n");
        return -1;
    }
}
// 从文件描述符fd中读取count个字节到buffer,成功返回读取的字节数，失败返回-1
int32 sysRead(uint32 fd, void* buffer, uint32 count){
    int32 retVal = -1;
    if(fd < 0 || fd == STD_OUT || fd == STD_ERR){
        printk("FS_SYS_READ: fd error\n");
        return -1;
    }else if(fd == STD_IN){
        char* mBuffer = buffer;
        uint32 readByteCnt = 0;
        while(readByteCnt < count){
            *mBuffer = ioqGetChar(&KBD_BUFFER);
            readByteCnt++;
            mBuffer++;
        }
        retVal = (readByteCnt == 0 ? -1 : (int32)readByteCnt);
    }else{
        uint32 gfd = fdLocal2Global(fd);
        retVal = fileRead(&fileTable[gfd], buffer, count);
    }
    ASSERT(buffer != NULL);
    
    return retVal;
}
// 重置用于文件读写操作的偏移指针，成功时返回偏移量，失败时返回-1
int32 sysLSeek(int32 fd, int offset, uint8 whence){
    if(fd < 0){
        printk("FS_LSEEK: fd error!\n");
        return -1;
    }
    ASSERT(whence > 0 && whence < 4);
    uint32 gfd = fdLocal2Global(fd);
    struct File* file = &fileTable[gfd];
    int32 newPos = 0;
    int32 fileSize = (int32)file->fdInode->dataSize;
    // 注意：offset可正可负
    switch (whence)
    {
    case SEEK_SET:
        newPos = offset;
        break;
    
    case SEEK_CUR:
        newPos = (int32)file->fdPos + offset;
        break;
    case SEEK_END:
        newPos = fileSize + offset;
        break;
    }
    if(newPos < 0 || newPos > (fileSize - 1)){
        return -1;
    }
    file->fdPos = newPos;
    return file->fdPos;
}
/* 关闭文件描述符fd指向的文件，成功返回0， 否则返回-1 */
int32 sysClose(int32 fd){
    int32 retVal = -1;
    if(fd > 2){
        uint32 globalFd = fdLocal2Global(fd);
        retVal = fileClose(&fileTable[globalFd]);
        runningThread()->fdTable[globalFd] = -1;
    }
    return retVal;
}
// 删除文件，成功返回0，失败返回-1
int32 sysUnlink(const char* pathname){
    // 检查需要删除的文件时候存在
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    int inodeNum = searchFile(pathname, &searchRecord);
    ASSERT(inodeNum != 0);
    if(inodeNum == -1){
        printkf("file '%s' not found!\n");
        return -1;
    }
    if(searchRecord.fileType == FT_DIR){
        printk("can't delete direcotry with unlink\n");
        return -1;
    }
    // 检查文件是否已经打开
    uint32 fileIndex = 0;
    while(fileIndex < MAX_FILE_OPEN_NUM){
        if(fileTable[fileIndex].fdInode != NULL && fileTable[fileIndex].fdInode->inodeNum == inodeNum){
            break;
        }
        fileIndex++;
    }
    if(fileIndex < MAX_FILE_OPEN_NUM){
        dirClose(searchRecord.parentDir);
        printkf("file '%s' is in use, not allow to delete!\n");
        return -1;
    }
    void* ioBuff = sys_malloc(SECTOR_SIZE * 2);
    if(ioBuff == NULL){
        printk("FS_SYS_UNLINK: malloc io buffer faile!\n");
        return -1;
    }
    struct Dir* parentDir = searchRecord.parentDir;
    deleteDirEntry(currentPart, parentDir, inodeNum, ioBuff);
    inodeRelease(currentPart, inodeNum);
    sys_free(ioBuff);
    dirClose(searchRecord.parentDir);
    return 0;
}
// 创建目录pathName, 成功返回0，失败返回-1
int32 sysMkdir(const char* pathName){
    uint8 rollbackStep = 0; // 用于失败时的回滚操作
    void* ioBuffer = sys_malloc(SECTOR_SIZE * 2);
    if(ioBuffer == NULL){
        printk("FS_SYS_MKDIR: alloc io buffer failed!\n");
        return -1;
    }
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    int32 inodeNum = -1;
    inodeNum = searchFile(pathName, &searchRecord);
    if(inodeNum != -1){
        // 找到同名文件夹，返回
        printkf("FS_SYS_MKDIR: file or directory '%s' exist!\n", pathName);
        rollbackStep = 1;
        goto rollback;
    }else{
        // 没有找到时，出现两种状况，一、文件不存在，继续执行，二、中间某个路劲不存在，返回返回失败
        uint32 pathNameDepth = pathDepthCnt((char*)pathName);
        uint32 searchDepth = pathDepthCnt(searchRecord.searchPath);
        printkf("PND=%d, SPD=%d\n", pathNameDepth, searchDepth);
        if(pathNameDepth != searchDepth){
            printkf("FS_SYS_MKDIR: can't access '%s', not d directory, subpath '%s' is't exist\n", pathName, searchRecord.searchPath);
            rollbackStep = 1;
            goto rollback;
        }
    }
    struct Dir* parentDir = searchRecord.parentDir;
    // 构建目录名字，从searchRecord里获取，避免出现'/'
    char* dirName = strrchr(searchRecord.searchPath,'/') + 1;
    inodeNum = inodeBitmapAlloc(currentPart);
    if(inodeNum == -1){
        printk("FS_SYS_MKDIR: alloc inode failed!\n");
        rollbackStep = 1;
        goto rollback;
    }
    struct Inode newInode;
    inodeInit(inodeNum, &newInode);
    struct BlockInfo blockInfo;
    blockInfo.blockLBA = blockAlloc(currentPart);
    if(blockInfo.blockLBA == -1){
        printk("FS_SYS_MKDIR: alloc block failed!\n");
        rollbackStep = 2;
        goto rollback;
    }
    blockInfo.blockIndex = 0;
    inodeAddBlock(currentPart, &newInode, &blockInfo);
    memset(ioBuffer, 0, SECTOR_SIZE * 2);
    struct DirEntry* dirEntryPtr =(struct DirEntry*)ioBuffer;

    memcpy(dirEntryPtr->fileName, ".", 1);
    dirEntryPtr->inodeNum = inodeNum;
    dirEntryPtr->fileType = FT_DIR;

    dirEntryPtr++;
    memcpy(dirEntryPtr->fileName, "..", 2);
    dirEntryPtr->fileType = FT_DIR;
    dirEntryPtr->inodeNum = parentDir->inode->inodeNum;
    ideWrite(currentPart->disk, newInode.blockTable[0], ioBuffer, 1);
    newInode.dataSize = 2 * currentPart->superBlock->dirEntrySize;

    /* 在父目录中添加自己的目录项 */
    struct DirEntry newDirEntry;
    memset(&newDirEntry, 0, sizeof(struct DirEntry));
    createDirEntry(dirName, inodeNum, FT_DIR, &newDirEntry);
    // 清空buffer,同步父目录
    memset(ioBuffer, 0, SECTOR_SIZE * 2);
    if(!syncDirEntry(parentDir, &newDirEntry, ioBuffer)){
        printk("FS_SYS_MKDIR: sync dir to disk failed!\n");
        rollbackStep = 2;
        goto rollback;
    }
    // 同步父目录的inode到硬盘
    memset(ioBuffer, 0, SECTOR_SIZE * 2);
    inodeSync(currentPart, parentDir->inode, ioBuffer);
    // 同步新目录的inode到硬盘
    memset(ioBuffer, 0, SECTOR_SIZE * 2);
    inodeSync(currentPart, &newInode, ioBuffer);
    // 同步inode位图
    bitmapSync(currentPart, inodeNum, INODE_BITMAP);
    sys_free(ioBuffer);
    dirClose(searchRecord.parentDir);
    return 0;

rollback:
    switch (rollbackStep)
    {
    case 2:
        setBitmap(&currentPart->inodeBitmap, inodeNum, 0);
    case 1:
    dirClose(searchRecord.parentDir);
    }
    sys_free(ioBuffer);
    return -1;
}
// 打开目录，成功后返回目录指针，失败后返回NULL
struct Dir* sysOpendir(const char* name){
    ASSERT(strlen(name) < MAX_PATH_LEN);
    // 无需判断name[2],因为根目录的父目录为自己
    if(name[0] == '/' && (name[1] == 0 || name[1] == '.')){
        return &rootDir;
    }
    // 检查需要打开的目录是否存在
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    int inodeNum = searchFile(name, &searchRecord);
    struct Dir* retDir = NULL;
    if(inodeNum == -1){
        printkf("In '%s', sub path '%s' not exist\n", name, searchRecord.searchPath);
    }else{
        if(FT_FILE == searchRecord.fileType){
            printkf("'%s' is regular file!\n", name);
        }else if(FT_DIR == searchRecord.fileType){
            retDir = dirOpen(currentPart, inodeNum);
        }
    }
    dirClose(searchRecord.parentDir);
    return retDir;
}
// 读取一个目录项，成功后返回其目录项地址，失败返回NULL
struct DirEntry* sysReadDirEntry(struct Dir* dir){
    return readDirEntry(dir);
}
// 把目录的dirPos置0，以此实现目录回绕
void sysRewinddir(struct Dir* dir){
    dir->dirPos = 0;
}
// 关闭目录，成功返回0，失败返回-1
int32 sysCloseDir(struct Dir* dir){
    int32 retVal = -1;
    if(dir != NULL){
        dirClose(dir);
        retVal = 0;
    }
    return retVal;
}
// 删除空目录, 成功返回0，失败返回-1
int32 sysRmdir(const char* pathName){
    // 1、检查要删除的目录是否存在
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    int32 inodeNum = searchFile(pathName, &searchRecord);
    ASSERT(inodeNum != 0);
    int32 retVal = -1;
    if(inodeNum == -1){
        printkf("In '%s', sub path '%s' not exist!\n", pathName, searchRecord.searchPath);
    }else{
        if(searchRecord.fileType == FT_FILE){
            printkf("'%s' is regular file!\n", pathName);
        }else{
            struct Dir* dir = dirOpen(currentPart, inodeNum);
            if(!dirIsEmpty(dir)){
                printkf("dir '%s' is not empty, can't delete nonempty dir!\n", pathName);
            }else{
                if(!dirRemove(searchRecord.parentDir, dir)){
                    retVal = 0;
                }
            }
            dirClose(dir);
        }
    }
    dirClose(searchRecord.parentDir);
    return retVal;
}
// 获取父目录的inode号
static uint32 parentDirInode(uint32 childInodeNum, void* ioBuffer){
    struct Inode* childInode = inodeOpen(currentPart, childInodeNum);
    uint32 blockLBA = childInode->blockTable[0];
    // 目录"."、".."在0的位置
    inodeClose(childInode);
    ideRead(currentPart->disk, blockLBA, ioBuffer, 1);
    struct DirEntry* dirEntryPtr = (struct DirEntry*)ioBuffer;\
    // ".."是父目录
    return dirEntryPtr[1].inodeNum;
}
// 在inode编号为parentInodeNum的目录中查找inode号为childInodeNum的子目录名称, 成功返回0，失败返回-1
static int childDirName(uint32 parentInodeNum, uint32 childInodeNum, char* path, void* ioBuffer){
    struct Inode* parentInode = inodeOpen(currentPart, parentInodeNum);
    uint32 blockCnt = 12;
    uint32 allBlocks[140] = {0};
    inodeReadBlocks(currentPart, parentInode, allBlocks);
    if(parentInode->blockTable[12] != 0){
        blockCnt = 140;
    }
    inodeClose(parentInode);
    struct DirEntry* dirEntryPtr = (struct DirEntry*)ioBuffer;
    uint32 dirEntrySize = currentPart->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_SIZE / dirEntrySize;
    uint32 tableIndex = 0;
    while(tableIndex < blockCnt){
        if(allBlocks[tableIndex] != 0){
            ideRead(currentPart->disk, allBlocks[tableIndex], ioBuffer, 1);
            uint32 dirEntryIndex = 0;
            while(dirEntryIndex < dirEntryCntPerSec){
                if((dirEntryPtr + dirEntryIndex)->inodeNum == childInodeNum){
                    strcat(path,"/");
                    strcat(path, (dirEntryPtr + dirEntryIndex)->fileName);
                    return 0;
                }
                dirEntryIndex++;
            }
        }
        tableIndex++;
    }
    return -1;
}
// 把当前工作目录绝对路径写入buffer, size是buffer的大小，当buffer为NULL时，由操作系统分配存储工作路径的空间并返回地址
char* sysGetCwd(char* buffer, uint32 size){
    void* ioBuffer = sys_malloc(SECTOR_SIZE);
    if(ioBuffer == NULL){
        return NULL;
    }
    struct TaskStruct* currentThread = runningThread();
    uint32 parentInodeNum = 0;
    uint32 childInodeNum = currentThread->cwdInodeNum;
    if(childInodeNum == 0){
        buffer[0] = '/';
        buffer[1] = 0;
        return buffer;
    }
    memset(buffer, 0, size);
    // 全路径缓冲区
    char fullPathReverse[MAX_PATH_LEN] = {0};
    // 从下往上逐层找父目录，直到找到根目录为止
    while(childInodeNum != 0){
        parentInodeNum = parentDirInode(childInodeNum, ioBuffer);
        // 没有找到名称，失败返回
        if(childDirName(parentInodeNum, childInodeNum, fullPathReverse, ioBuffer) == -1){
            sys_free(ioBuffer);
            return NULL;
        }
        childInodeNum = parentInodeNum;
    }
    // 此时，查找到的路径是反着的，举例：实际路径"/a/b"，fullPathReverse中为"/b/a"
    char* lastSlash;    // 记录最后一个'/'地址
    while ((lastSlash = strrchr(fullPathReverse, '/'))){
        uint32 len = strlen(buffer);
        strcpy(buffer + len, lastSlash);
        // 在fullPathReverse中添加0，作为字符串结束标志，相当与去掉了已经处理的部分
        *lastSlash = 0;
    }
    sys_free(ioBuffer);
    return buffer;
    
}
// 更改当前工作路径为绝对路径path，成功返回0，失败返回-1
int32 sysChdir(const char* path){
    int32 retVal = -1;
    struct PathSearchRecord searchRecord;
    memset(&searchRecord,0, sizeof(struct PathSearchRecord));
    int32 inodeNum = searchFile(path, &searchRecord);
    if(inodeNum != -1){
        if(searchRecord.fileType == FT_DIR){
            runningThread()->cwdInodeNum = inodeNum;
            retVal = 0;
        }else{
            printkf("FS_SYS_CHDIR: '%s' is file or other!\n", path);
        }
    }
    dirClose(searchRecord.parentDir);
    return retVal;
}
// buf中填充文件结构信息，成功返回0，失败返回-1
int32 sysStat(const char* path, struct Status* buffer){
    // 处理根目录
    if(!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")){
        buffer->inodeNum = 0;
        buffer->size = rootDir.inode->dataSize;
        buffer->ctime = rootDir.inode->creatTime;
        buffer->mtime = rootDir.inode->modifyTime;
        buffer->fileType = FT_DIR;
        return 0;
    }
    int32 retVal = -1;
    struct PathSearchRecord searchRecord;
    memset(&searchRecord, 0, sizeof(struct PathSearchRecord));
    int inodeNum = searchFile(path, &searchRecord);
    if(inodeNum != -1){
        struct Inode* objInode = inodeOpen(currentPart, inodeNum);
        buffer->inodeNum = objInode->inodeNum;
        buffer->size = objInode->dataSize;
        buffer->ctime = objInode->creatTime;
        buffer->mtime = objInode->modifyTime;
        buffer->fileType = searchRecord.fileType;
        retVal = 0;
    }else{
        printkf("FS_SYS_STAT: '%s' not found!\n", path);
    }
    dirClose(searchRecord.parentDir);
    return retVal;
}