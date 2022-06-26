#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#define PAGE_SZIE 4096
/* 内存管理设计：
 * 至多管理1GB内存，内核空间与用户空间平均分配，每个空间至多有512MiB内存空间。
 * 因此，需要需要为每个空间预分配4个页（16KiB）的位图空间（每页位图能管理128MiB的内存空间）
 * 共需要16页共64KiB的内存空间，其中内核物理内存位图16KiB, 内核虚拟空间位图16KiB,用户物理内存位图空间16KiB,用户虚拟地址空间位图16KiB
 * 位图分配顺序：内核物理内存位图，内核虚拟内存位图，用户物理内存位图，用户虚拟内存位图
 * 
 */

// 这里的地址需要十分注意，错误的地址会导致string库调用setmem时引发GP（一般性保护异常）
// 内核虚拟地址开始的位置,此处需要加上低1MB和高1MB内存空间
#define KERNEL_VADDR_START 0xC0200000 

// 内存信息个数入口
#define ARDS_ENTRY_COUNT_PADDR 0x00007E00
// 内存信息表地址
#define ARDS_ENTRY_PADDR ARDS_ENTRY_COUNT_PADDR + 4

// 内核物理内存位图保存的物理位置
#define KERNEL_PMEM_BITMAP_VADDR 0xC01EF000
// 内核虚拟地址池位图保存的物理位置
#define KERNEL_VMEM_BITMAP_VADDR 0xC01F3000

// 用户物理内存位图的虚拟地址
#define USER_PMEM_BITMAP_VADDR 0xC01F7000
// 用户虚拟地址池位图的物理地址
#define USER_VMEM_BITMAP_VADDR 0xC01FB000

struct ARDS{
    uint32 baseVaddrLow;
    uint32 baseVaddrHigh;
    uint32 lengthLow;
    uint32 lengthHigh;
    uint32 type;
    uint32 sign;
};
// 内存池类型枚举，用来判断哪个内存池
enum PoolFlag{
    PF_KERNEL = 1, // 内核内存池
    PF_USER = 2 // 用户内存池
};
// 页属性
#define PAGE_P_0  0 // 页表项或页目录项存在属性，表示在内存中不存在
#define PAGE_P_1  1 // 页表项或页目录项存在属性，表示在内存中存在
#define PAGE_RW_R   0 // R/W属性，读、执行
#define PAGE_RW_W   2 // R/W属性，读、写、执行
#define PAGE_US_S   0   // U/S属性位，系统级
#define PAGE_US_U   4   // U/S属性位。用户级 
// 虚拟地址结构
struct VirtualAddr
{
    struct Bitmap vaddrBitmap;
    uint32 vaddrStart;
};
// 内存池结构
struct Pool
{
    struct Bitmap bitmap;
    uint32 phyaddrStart;
    uint32 poolSize;
};
// 并向外部导出标号
extern struct Pool kernelPool, userPool;

void initMem(void);
uint32* getPdePtr(uint32 vaddr);
uint32* getPtePtr(uint32 vaddr);
void* mallocPage(enum PoolFlag pf, uint32 pageCount);
void* allocKernelPages(uint32 pageCount);
#endif