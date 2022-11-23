#ifndef FS_FILE_H
#define FS_FILE_H

#include "stdint.h"
#include "inode.h"
#include "dir.h"

// 系统可打开的最大文件数
#define MAX_FILE_OPEN_NUM   32
// 向前声明
struct Dir;
/* 文件结构 */
struct File{
    // 记录当前文件的偏移位置
    uint32 fdPos;
    // 记录文件的打开属性
    uint32 fdFlag;
    struct Inode* fdInode;
};
// 标准输入输出描述符
enum STD_FD{
    STD_IN = 0,     // 标准输入
    STD_OUT = 1,    // 标准输出
    STD_ERR = 2     // 标准错误
};
// 位图类型
enum BitmapType {
    INODE_BITMAP,   // inode位图
    BLOCK_BITMAP    // 块位图
};
/* 导出打开的文件表 */
extern struct File fileTable[MAX_FILE_OPEN_NUM];
int32 blockBitmapAlloc(struct Partition* part);
// 将内存中bitmap的bitIndex位所在的512字节更新后写入硬盘
void bitmapSync(struct Partition* part, uint32 bitIndex, uint8 bitmap);
// 分配一个inode并返回inode号
int32 inodeBitmapAlloc(struct Partition* part);
// 将全局文件描述符下标安装到任务自己的PCB中，成功返回任务自己的FD index,失败返回-1
int32 installFdInPCB(int32 fileTableIndex);
// 在fileTable中查找空位并返回index，没有空位时返回-1
int32 getFileTableIndex(void);
// 创建文件，成功则返回文件描述符，否则返回-1
int32 fileCreate(struct Dir* parentDir, char* fileName, uint8 flag);
// 打开编号为inodeNum的inode对应的文件，成功则返回文件描述符，失败则返回-1
int32 fileOpen(uint32 inodeNum, uint8 flag);
// 把buffer中count个字节写入file, 成功返回写入的字节数，失败返回-1
int32 fileWrite(struct File* file, const void* buffer, uint32 count);
// 从file中读取count个字节写入buffer, 成功返回读取到的字节数，失败返回-1
int32 fileRead(struct File* file, void* buffer, uint32 count);
// 关闭文件, 成功关闭返回0，失败返回-1
int32 fileClose(struct File* file);
#endif