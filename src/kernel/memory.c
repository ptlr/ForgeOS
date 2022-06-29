#include "memory.h"
#include "stdio.h"
#include "print.h"
#include "debug.h"
#include "string.h"

// 获取PDE的index
#define PDE_INDEX(vaddr) ((vaddr & 0xFFC00000) >> 22)
// 获取PTE的index
#define PTE_INDEX(vaddr) ((vaddr & 0x003FF000) >> 12)

struct VirtualAddr kernelVaddr;
// 定义内核内存池和用户内存池
struct Pool kernelPool, userPool;
/* 
 * 在pf表示的内存池中分配pageCount个虚拟页，成功返回虚拟页的开始地址，失败返回NULL
 */
static void* getVaddrPages(enum PoolFlag pf, uint32 pageCount){
    int vaddrStart = 0;
    int bitIndexStart = -1;
    uint32 count = 0;
    // 分配内核空间的虚拟页
    if( PF_KERNEL == pf){
        bitIndexStart = scanBitmap(&kernelVaddr.vaddrBitmap, pageCount);
        if(-1 == bitIndexStart) return NULL;
        while (count < pageCount)
        {
            setBitmap(&kernelVaddr.vaddrBitmap, bitIndexStart + count++, 1);
        }
        vaddrStart = kernelVaddr.vaddrStart + bitIndexStart * PAGE_SZIE;
        
    }else{
    // 分配用户空间的虚拟页
    }
    return (void*)vaddrStart;
}

/* 分配内存时，需要关联虚拟地址和物理地址，此时，需要更新页目录表（PD）和页表（PT）
 * 由于操作系统只有一个页目录表（PD）和至多1024个页表（PT）
 */
/* 获取虚拟地址对应的pte指针
 * 
 */
uint32* getPtePtr(uint32 vaddr){
    /* 获取pte指针步骤：
     * 1、访问页目录表（在该学习项目中，0xFFC,最后一项指向了页目录表）
     * 2、在页目录表中找到vaddr对应的页表的页表项（PDE）：由于第1024个指向了页目录表本身，vaddr中的高10位可以当做，页表的索引。
     * 3、最后12位则是由vaddr的pte部分构成，由于最后12位会被当成页内偏移，所以需要手动乘4
     */
    uint32* pte = (uint32*)(0xFFC00000 + ((vaddr & 0xFFC00000) >> 10) + PTE_INDEX(vaddr) * 4);
    return pte;
}

/* 获取虚拟地址vaddr对应的PDE指针
 * 由于第1024个页表项指向了页目录表本身，故高20位表示访问到了最后一个目录项
 * 利用对应PDE的index乘*4获取需要更新的PDE地址
 */
uint32* getPdePtr(uint32 vaddr){
    // 
    uint32* pde  = (uint32*)((0xFFFFF000) + PDE_INDEX(vaddr) * 4);
    return pde;
}
/* 在memPool对应的物理内存池中分配一个物理页
 * 成功返回页框的物理地址，失败则放回NULL
 * 提示：开始分页管理模式后，虚拟地址连续，但分配到的物理内存页可以不连续，这样，可以提高物理内存的使用率，因此只需要每次分配一个
 */
static void* palloc(struct Pool* memPool){
    int bitIndex =  scanBitmap(&memPool->bitmap, 1);
    if(-1 == bitIndex) return NULL;
    setBitmap(&memPool->bitmap, bitIndex, 1);
    uint32 pagePaddr = ((bitIndex * PAGE_SZIE) + memPool->phyaddrStart);
    return (void*)pagePaddr;
}
// 向页目录表中添加虚拟内存与物理地址的映射
static void addPageTable(void* vaddr, void* paddr){
    uint32 mVaddr = (uint32)vaddr, pagePaddr = (uint32)paddr;
    uint32* pde = getPdePtr(mVaddr);
    
    uint32* pte = getPtePtr(mVaddr);
    printf("PDE = 0x%x\n", *pde);
    printf("PTE = 0x%x\n", *pte);
    // 判断目录页是否存在，如果已存在表示该表已存在
    if(*pde & 0x00000001){
        // 如果pte不存在
        if(!(*pte & 0x00000001)){
            *pte = (pagePaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1);
        }else{
            // 目前不应该执行到这里
            logError("PTE repeat");
            PANIC("PTE repeat");
            *pte = (pagePaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1);
        }
    }else{
        // 如果不存在，则创建新的pde后再创建页表项
        // 从内核物理内存池中分配一页
        uint32 pdePaddr = (uint32)palloc(&kernelPool);
        *pde = pdePaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1;
        memset((void*)((int)pte & 0xFFFFF000),0,  PAGE_SZIE);
        ASSERT(!(*pte & 0x00000001));
        *pte = pagePaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1;
    }
}
// 分配pageCount个页空间，成功返回起始虚拟地址，失败返回NULL
void* mallocPage(enum PoolFlag pf, uint32 pageCount){
    void* vaddrStart = getVaddrPages(pf, pageCount);
    if(vaddrStart == NULL) return NULL;
    uint32 vaddr =  (uint32)vaddrStart, count = pageCount;
    struct Pool* memPool = pf & PF_KERNEL ? &kernelPool : &userPool;
    while (count--)
    {
        void* pagePaddr = palloc(memPool);
        if(NULL == pagePaddr) return NULL;
        addPageTable((void*)vaddr, pagePaddr);
        vaddr += PAGE_SZIE;
    }
    return vaddrStart;
}
// 分配物理页
void* allocKernelPages(uint32 pageCount){
    void* vaddr = mallocPage(PF_KERNEL, pageCount);
    if(NULL != vaddr){
        memset(vaddr, 0, PAGE_SZIE);
    }
    return vaddr;
}
static void initMemPool(uint32 maxMemSize)
{
    printf("    *init memory pool\n");
    uint32 sizeMB = maxMemSize / 1024 / 1024;
    if(sizeMB > 1024){
        logWaring("Memory size Biger than 1024MiB.");
        maxMemSize = 1024 * 1024 * 1024;
    }
    /* 计算出可用的内存数：
     * 操作系统内核被放在了1MB-2MB位置，紧接着安排操作系统的页目录表，紧接着操作系统的第一个页表。
     * 传入的maxMemSize是查询到的，最大的一段内存，它一般从1MB开始，因此，需要额外减去1MB
     * 页目录表中的第0个、第768个页目录项指向同一个页；第769~1022个需要占用254个页；第1023个页目录指向页表本身；故需要为系统保留共256个页
     * 
     */
    uint32 usedMemSize = 0x100000 + 256 * PAGE_SZIE;
    // 计算出可用的页数
    uint32 freeMemSize = maxMemSize - usedMemSize;
    uint16 freePages = freeMemSize / PAGE_SZIE;
    /* 平均分配内核页数和用户页数，可用页为奇数时，为用户多分一页
     */
    // 除法：不为零时向下取整
    uint32 kernelFreePages = freePages / 2;
    // 用减法：可用页为奇数页时，不用复杂操作就可以实现为用户多分一页。
    uint32 userFreePages = freePages - kernelFreePages;
    // 初始化内核内存池
    /* 计算内核内存池的物理地址开始位置
     * 已经用去和预先分配出去的之外，需要加上低1MB内存
     */
    kernelPool.phyaddrStart = 0x100000 + usedMemSize;
    kernelPool.poolSize = kernelFreePages * PAGE_SZIE;
    kernelPool.bitmap.length = kernelFreePages / 8;
    kernelPool.bitmap.bits = (void*)KERNEL_PMEM_BITMAP_VADDR;

    // 初始化用户内存池

    userPool.phyaddrStart = kernelPool.phyaddrStart + kernelFreePages * PAGE_SZIE;
    userPool.poolSize = userFreePages * PAGE_SZIE;
    userPool.bitmap.length = userFreePages / 8;
    userPool.bitmap.bits = (void*)USER_PMEM_BITMAP_VADDR;
    // 输出简单的信息
    printf("     kernel pool: PADDR_START = 0x%x, BITMAP_VADDR = 0x%x\n",kernelPool.phyaddrStart, kernelPool.bitmap.bits);
    printf("     user   pool: PADDR_START = 0x%x, BITMAP_VADDR = 0x%x\n",userPool.phyaddrStart, userPool.bitmap.bits);
    // 初始化bitmap
    initBitmap(&kernelPool.bitmap);    
    initBitmap(&userPool.bitmap);
    //初始化内核对应的虚拟地址池
    kernelVaddr.vaddrStart = KERNEL_VADDR_START;
    kernelVaddr.vaddrBitmap.length = kernelPool.bitmap.length;
    kernelVaddr.vaddrBitmap.bits = (void*)KERNEL_VMEM_BITMAP_VADDR;
    printf("     kernel vaddr: VADDR_START = 0x%x, BITMAP_PADDR = 0x%x\n", kernelVaddr.vaddrStart, kernelVaddr.vaddrBitmap.bits);
}

void initMem(void)
{
    printf("[09] init memory\n");
    int count = *((uint32*)ARDS_ENTRY_COUNT_PADDR);
    struct ARDS ards[count];
    uint32 maxIndex = 0;
    uint64 length = 0;
    uint64 maxLength = 0;
    for(int index = 0; index < count; index++)
    {
       uint32 mmeAddr = (uint32)(ARDS_ENTRY_PADDR + index * sizeof(struct ARDS));
       ards[index] = *(struct ARDS*)mmeAddr;
       char bh[9] = "";
       char bl[9] = "";
       char lh[9] = "";
       char ll[9] = "";
       uint2HexStr(bh, ards[index].baseVaddrHigh, 8);
       uint2HexStr(bl, ards[index].baseVaddrLow, 8);
       uint2HexStr(lh, ards[index].lengthHigh, 8);
       uint2HexStr(ll, ards[index].lengthLow, 8);
       printf("     BASE = 0x%s%s, LENGHT = 0x%s%s, TYPE = %d\n", bh, bl, lh, ll, ards[index].type);
       length = (((uint64)ards[index].lengthHigh) << 32) + (uint64)ards[index].lengthLow;
       if(length > maxLength){
           maxLength = length;
           maxIndex = index;
       }
    }
    /*
     * 此次学习和开的发操作系统为32位，即使在4GiB的也可用32位数表示，直接传入LengthLow即可。
     */
    initMemPool(ards[maxIndex].lengthLow);
}
