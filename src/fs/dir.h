#ifndef FS_IDR_H
#define FS_IDR_H

#include "stdint.h"
#include "file.h"
#include "inode.h"
#include "fs.h"

#define MAX_FILE_NAME_LEN       192

/* 目录结构 */
struct Dir{
    struct Inode* inode;
    uint32 dirPos;      // 记录目录内的偏移
    uint8 dirBuffer[512];   // 目录缓存
};
struct DirEntry{
    char fileName[MAX_FILE_NAME_LEN];
    uint32 inodeNum;
    enum FileType fileType;
};
// 导出根目录
extern struct Dir rootDir;
// 打开根目录
void openRootDir(struct Partition* part);
// 在分区part上打开inode位inodeNum的目录并返回目录指针
struct Dir* dirOpen(struct Partition* part, uint32 inodeNum);
/* 关闭目录 */
void dirClose(struct Dir* dir);
/* 在分区part的dir目录中寻找名为name的文件或目录
 * 找到返回true，并把目录项写入到dirEntry中，否则返回false
 */
bool searchDirEntry(struct Partition* part, struct Dir* dir, const char* name, struct DirEntry* dirEntry);
/* 在内存中初始化目录项 */
void createDirEntry(char* fileName, uint32 inodeNum, uint8 fileType, struct DirEntry* dirEntry);
/* 将目录项dirEntry写入父目录parentDir中， ioBuffer由主调函数提供 */
bool syncDirEntry(struct Dir* parentDir, struct DirEntry* dirEntry, void* ioBuffer);
// 读取目录，成功返回目录项，失败返回NULL
struct DirEntry* readDirEntry(struct Dir* dir);
// 删除part父目录为parentDir编号为inodeNum的目录项
bool deleteDirEntry(struct Partition* part, struct Dir* parentDir, uint32 inodeNum, void* ioBuff);
// 判断目录是否为空
bool dirIsEmpty(struct Dir* dir);
// 在父目录parentDir中删除子目录childDir对应的目录项
int32 dirRemove(struct Dir* parentDir, struct Dir* childDir);
#endif