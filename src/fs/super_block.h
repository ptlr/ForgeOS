#ifndef FS_SUPER_BLOCK_H
#define FS_SUPER_BLOCK_H
#include "stdint.h"
// 超级块，用于保存分区文件系统信息
struct SuperBlock{
    uint32 magic;                   // 用于标识文件系统类型，支持多文件系统的操作系统通过此标志区分文件系统类型
    uint32 sectorCount;             // 本分区中sector数量
    uint32 inodeCount;              // 本分区中inode数量
    uint32 partLbaBase;             // 本分区起始的LBA地址

    uint32 blockBitmapLba;          // 块位图开始的LBA地址
    uint32 blockBitmapSectorCnt;    // 块位图本身占的扇区数

    uint32 inodeBitmapLba;          // i结点位图开始的扇区LBA
    uint32 inodeBitmapSectorCnt;    // i结点位图占用扇区数

    uint32 inodeTableLba;           // inode表开始扇区LBA
    uint32 inodeTableSectorCnt;     // inode表占用的扇区数量

    uint32 dataStartLba;            // 数据开始扇区的LBA号
    uint32 rootInodeNum;            // 根目录inode号
    uint32 dirEntrySize;            // 目录项大小
    uint8  padding[460];            // 加上460字节，凑够512个字节，共一个扇区大小
} __attribute__ ((packed));
#endif