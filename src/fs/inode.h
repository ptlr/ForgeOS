#ifndef FS_INODE_H
#define FS_INODE_H

#include "stdint.h"
#include "list.h"
#include "constant.h"
#include "ide.h"
#include "block.h"
#define BLOCK_TABLE_LEN 13
enum BlockType{
    L0_BLOCK = 1,       // 0~11，直接块
    L1_BLOCK,           // 12，一级间接块
    L2_BLOCK,           // 13， 二级间接块
    L3_BLOCK            // 14， 三级间接块
};
/* inode结构 */
struct Inode{
    uint32 inodeNum;            // inode编号
    uint32 dataSize;            // 文件时，记录文件大小，目录时，记录目录项大小之和
    uint32 opentCnt;            // 打开次数
    uint32 modifyTime;      // 最后更新时间
    uint32 creatTime;           // 文件创建时间
    bool   writeDeny;           // 写文件不能并行，写文件前检查该标识
    uint32 blockTable[BLOCK_TABLE_LEN];        // 0~11是直接块，12是一级间接块指针，13是二级间接块指针
    struct ListElem inodeTag;   // inode标识
};
/* 用来存储inode位置 */
struct InodePosition{
    bool twoSector;     // inode是否跨扇区
    uint32 sectorLBA;   // inode所在的扇区
    uint32 offset;      // inode在扇区内的偏移量
};
// 向前申明
struct BlockInfo;
// 初始化inode
void inodeInit(uint32 inodeNum, struct Inode* inode);
// 根据inode号返回inode信息
struct Inode* inodeOpen(struct Partition* part, uint32 inodeNum);
// 关闭或减少inode的打开次数
void inodeClose(struct Inode* inode);
// 将硬盘上的inode清空
void inodeDelete(struct Partition* part, uint32 inodeNum, void* ioBuff);
// 把inode写入分区part
void inodeSync(struct Partition* part, struct Inode* inode, void* buffer);
// 回收inode本身和inode对应的数据块
void inodeRelease(struct Partition* part, uint32 inodeNum);
// 在inode的直接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL0Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
bool inodeRemoveL0Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);

// 在inode的L1间接接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL1Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
// 把L1对应的索引表读取到ioBuff中
void inodeReadL1BlocksLBA(struct Partition* part, struct Inode* inode, void* ioBuff);
bool inodeRemoveL1Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);

// 在inode的L2间接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL2Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
void inodeReadL2BlocksLBA(struct Partition* part, struct Inode* inode, void* ioBuff);
bool inodeRemoveL2Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);

// 在inode的L3间接块中添加或移除块，成功返回true,失败返回false
bool inodeAddL3Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
void inodeReadL3BlocksLBA(struct Partition* part, struct Inode* inode, void* ioBuff);
bool inodeRemoveL3Block(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);

bool inodeAddBlock(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
void inodeReadBlocks(struct Partition* part, struct Inode* inode, uint32* blockTableBuff);
bool inodeRemoveBlock(struct Partition* part, struct Inode* inode, struct BlockInfo* blockInfo);
#endif