#include "memory.h"
#include "printk.h"
#include "debug.h"
#include "string.h"
#include "console.h"
#include "constant.h"
#include "thread.h"
#include "interrupt.h"
// 信息缓冲区
char msgBuff[128];
// 获取PDE的index
#define PDE_INDEX(vaddr) ((vaddr & 0xFFC00000) >> 22)
// 获取PTE的index
#define PTE_INDEX(vaddr) ((vaddr & 0x003FF000) >> 12)

// 内核内存块描述符数组
struct MemBlockDesc kernelMemBlockDescs[MEM_BLOCK_TYPE_COUNT];

struct VaddrPool kernelVaddr;
// 定义内核内存池和用户内存池
struct Pool kernelPool, userPool;
/* 初始化内存块描述符
 */
void memBlockDescInit(struct MemBlockDesc* descArray){
    uint16 descIndex;
    uint16 blockSize = 16;
    for(descIndex = 0; descIndex < MEM_BLOCK_TYPE_COUNT; descIndex++){
        descArray[descIndex].blockSize = blockSize;
        descArray[descIndex].blocksPerArena = (PAGE_SIZE - sizeof(struct Arena)) / blockSize;
        listInit(&descArray[descIndex].freeList);
        blockSize *= 2; // 为下一个内存块做准备
    }
}
/* 返回arena中第index个内存块的地址
 */
static struct MemBlock* arena2MemBlock(struct Arena* arena, uint32 index){
    return (struct MemBlock*)((uint32)arena + sizeof(struct Arena) + index * arena->desc->blockSize);
}
// 返回内存块所在的arena地址
static struct Arena* memBlock2Arena(struct MemBlock* memBlock){
    // 每个arena都是页，获取地址高20位返回即可
    return (struct Arena*)((uint32)memBlock & 0xFFFFF000);
}
// 在堆中申请size字节内存
void* sys_malloc(uint32 size){
    enum PoolFlag PF;
    struct Pool* memPool;
    uint32 poolSize;
    struct MemBlockDesc* descs;
    struct TaskStruct* currentThread = runningThread();
    // 根据线程pageDir是否被初始化判断用那个内存池
    if(currentThread->pageDir == NULL){
        //logWarning("allock\n");
        // 内核线程没有初始化pageDir
        PF = PF_KERNEL;
        poolSize = kernelPool.poolSize;
        memPool = &kernelPool;
        descs = kernelMemBlockDescs;
    }else{
        //logWarning("allocu\n");
        // pageDir被初始化的线程是用户线程
        PF = PF_USER;
        poolSize = userPool.poolSize;
        memPool = &userPool;
        descs = currentThread->userMemBlockDescs;
    }
    // 如果申请的内存大小大于内存池大小返回NULL
    if (!(size > 0 && size < poolSize)) return NULL;
    struct Arena* arena;
    struct MemBlock* memBlock;
    lockAcquire(&memPool->lock);
    // 如果申请的内存块超过1024B，直接分配页
    if(size > 1024){
        uint32 pageCount = DIV_ROUND_UP(size + sizeof(struct Arena), PAGE_SIZE);
        arena = mallocPage(PF, pageCount);
        if(arena != NULL){
            memset(arena, 0, pageCount * PAGE_SIZE);
            // 将desc置为NULL
            arena->desc = NULL;
            arena->count = pageCount;
            arena->large = true;
            lockRelease(&memPool->lock);
            return arena;
        }else{
            lockRelease(&memPool->lock);
            return NULL;
        }
    }
    // 分配小于等于1024B的内存块
    uint8 descIndex;
    // 找到对应的规格的index
    for(descIndex = 0; descIndex < MEM_BLOCK_TYPE_COUNT; descIndex++){
        if(size <= descs[descIndex].blockSize) break;
    }
    // 若对应的类型的freeList没有可用的内存块，则创建新的arena提供新的内存块
    if(listIsEmpty(&descs[descIndex].freeList)){
        arena = mallocPage(PF, 1);
        if(arena == NULL){
            lockRelease(&memPool->lock);
            return NULL;
        }
        memset(arena, 0, PAGE_SIZE);
        arena->desc = &descs[descIndex];
        arena->large = false;
        arena->count = descs[descIndex].blocksPerArena;
        uint32 blockIndex;
        enum IntrStatus oldStatus = intrDisable();
        // 将arena拆分称块添加到freeList中
        for(blockIndex = 0; blockIndex < arena->count; blockIndex++){
            memBlock = arena2MemBlock(arena, blockIndex);
            ASSERT(!listFind(&arena->desc->freeList, &memBlock->freeElem));
            listAppend(&arena->desc->freeList, &memBlock->freeElem);
        }
        setIntrStatus(oldStatus);
    }
    // 开始分配内存块
    memBlock = elem2entry(struct MemBlock, freeElem, listPop(&(descs[descIndex].freeList)));
    memset(memBlock, 0, descs[descIndex].blockSize);
    arena = memBlock2Arena(memBlock);
    arena->count--;
    lockRelease(&memPool->lock);
    return (void*)memBlock;
}
// 回收ptr指向的内存
void sys_free(void* ptr){
    ASSERT(ptr != NULL);
    if (ptr == NULL) return;
    enum PoolFlag pf;
    struct Pool* memPool;
    // 判断是进程还是线程
    if(runningThread()->pageDir == NULL){
        pf = PF_KERNEL;
        memPool = &kernelPool;
    }else{
        pf = PF_USER;
        memPool = &userPool;
    }
    lockAcquire(&memPool->lock);
    struct MemBlock* memBlock = ptr;
    struct Arena* arena = memBlock2Arena(memBlock);
    ASSERT(arena->large == 0 || arena->large == 1);
    if(arena->desc == NULL && arena->large == true){
        // 大于1024B的内存
        mfreePage(pf, arena, arena->count);
    }else{
        // 小于1024B的内存
        // 先将内存块放回收到freeList
        listAppend(&arena->desc->freeList, &memBlock->freeElem);
        // 判断arena中的内存块是否都是空闲
        if(++arena->count == arena->desc->blocksPerArena){
            uint32 blockIndex;
            for(blockIndex = 0; blockIndex < arena->desc->blocksPerArena; blockIndex++){
                struct MemBlock* memBlock = arena2MemBlock(arena, blockIndex);
                ASSERT(listFind(&arena->desc->freeList, &memBlock->freeElem));
                listRemove(&memBlock->freeElem);
            }
            mfreePage(pf, arena, 1);
        }
    }
    lockRelease(&memPool->lock);
}
/* 
 * 在pf表示的内存池中分配pageCount个虚拟页，成功返回虚拟页的开始地址，失败返回NULL
 */
static void* getVaddrPages(enum PoolFlag pf, uint32 pageCount){
    int vaddrStart = 0;
    int bitIndexStart = -1;
    uint32 count = 0;
    // 分配内核空间的虚拟页
    if( PF_KERNEL == pf){
        lockAcquire(&kernelVaddr.lock);
        bitIndexStart = scanBitmap(&kernelVaddr.vaddrBitmap, pageCount);
        if(-1 == bitIndexStart) return NULL;
        while (count < pageCount)
        {
            setBitmap(&kernelVaddr.vaddrBitmap, bitIndexStart + count++, 1);
        }
        vaddrStart = kernelVaddr.vaddrStart + bitIndexStart * PAGE_SZIE;
        lockRelease(&kernelVaddr.lock);
        
    }else{
        // 分配用户空间的虚拟页
        struct TaskStruct* currentThread = runningThread();
        bitIndexStart = scanBitmap(&currentThread->userProgVaddrPool.vaddrBitmap, pageCount);
        if(bitIndexStart == -1) return  NULL;
        while(count < pageCount){
            setBitmap(&currentThread->userProgVaddrPool.vaddrBitmap, bitIndexStart + count++, 1);
        }
        vaddrStart = currentThread->userProgVaddrPool.vaddrStart + bitIndexStart * PAGE_SIZE;
        // ToDo:: 这里理解不透彻，需要注意
        ASSERT((uint32)vaddrStart < (0xC0000000 - PAGE_SZIE));
    }
    return (void*)vaddrStart;
}
/* 在pf表示的内存池中释放从vaddr开头的pageCount个虚拟页地址
 * 
 */
static void vaddrPagesRemove(enum PoolFlag pf, uint32 vaddr, uint32 pageCount){
    uint32 bitIndexStart = 0;
    uint32 mVaddr = vaddr;
    uint32 count = 0;
    if(pf == PF_KERNEL){
        bitIndexStart = (mVaddr - kernelVaddr.vaddrStart) / PAGE_SIZE;
        while (count < pageCount)
        {
            setBitmap(&kernelVaddr.vaddrBitmap, bitIndexStart + count, 0);
            count++;
        }
    }else{
        struct TaskStruct* currentThread = runningThread();
        bitIndexStart = (mVaddr - currentThread->userProgVaddrPool.vaddrStart) / PAGE_SIZE;
        while(count < pageCount){
            setBitmap(&currentThread->userProgVaddrPool.vaddrBitmap, bitIndexStart + count++, 0);
        }    
    }
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
/* 将物理地址pagePaddr回收到物理内存池
 *
 */
static void pfree(uint32 pagePaddr){
    struct Pool* memPool;
    uint32  bitIndex = 0;
    if(pagePaddr >= userPool.phyaddrStart){
        memPool = &userPool;
    }else{
        memPool = &kernelPool;
    }
    bitIndex = (pagePaddr - memPool->phyaddrStart) / PAGE_SIZE;
    setBitmap(&memPool->bitmap, bitIndex, 0);
}

// 向页目录表中添加虚拟内存与物理地址的映射
static void addPageTable(void* vaddr, void* paddr){
    uint32 mVaddr = (uint32)vaddr;
    uint32 pagePaddr = (uint32)paddr;
    uint32* pde = getPdePtr(mVaddr);
    
    uint32* pte = getPtePtr(mVaddr);
    // 判断目录页是否存在，如果已存在表示该表已存在
    if(*pde & 0x00000001){
        // 如果pte不存在
        if(!(*pte & 0x00000001)){
            *pte = (pagePaddr | PAGE_US_U | PAGE_RW_W | PAGE_P_1);
        }else{
            // 目前不应该执行到这里
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
/* 去掉页表中虚拟地址vaddr的映射，只去掉vaddr对应的pte
 */
static void pageTablePteRemove(uint32 vaddr){
    uint32* ptePtr = getPtePtr(vaddr);
    /* 将pte中的P位置0
       编程技巧：&是按位和，不能使用PAGE_P_1,否则 1 & 0 = 0,导致ptePtr指向的数据错误
     */
    *ptePtr &= ~PAGE_P_1;
    asm volatile ("invlpg %0"::"m" (vaddr):"memory");//更新TLB, invlpg更新单条虚拟地址条目
}
// 分配pageCount个页空间，成功返回起始虚拟地址，失败返回NULL
void* mallocPage(enum PoolFlag pf, uint32 pageCount){
    void* vaddrStart = getVaddrPages(pf, pageCount);
    if(vaddrStart == NULL) return NULL;
    uint32 vaddr =  (uint32)vaddrStart;
    uint32 count = pageCount;
    struct Pool* memPool = pf & PF_KERNEL ? &kernelPool : &userPool;
    //logWarning("PRE_WAIT\n");
    while (count--)
    {
        void* pagePaddr = palloc(memPool);
        if(NULL == pagePaddr) return NULL;
        addPageTable((void*)vaddr, pagePaddr);
        vaddr += PAGE_SZIE;
    }
    return vaddrStart;
}
// 在pf表示的内存池中释放vaddr开始的pageCount个物理页
// ToDo: 这个函数可以优化，此处去掉了书中的ASSERT语句
void mfreePage(enum PoolFlag pf, void* vaddr, uint32 pageCount){
    uint32 pagePaddr;
    uint32 mVaddr = (uint32)vaddr;
    uint32 count = 0;
    ASSERT(pageCount >= 1 && mVaddr % PAGE_SIZE == 0);
    pagePaddr = getVaddrMapedPaddr(mVaddr);
    // 确保释放的物理内存在低1MB+高内核1MB + 4KB页目录表和第一个页表4KB
    ASSERT((pagePaddr % PAGE_SIZE) == 0 && pagePaddr >= 0x202000);
    // 判断vaddr对应的是虚拟用户地址还是内核地址
    if(pagePaddr >= userPool.phyaddrStart){
        // 用户内存池
        mVaddr -= PAGE_SIZE;
        while (count < pageCount)
        {
            mVaddr += PAGE_SIZE;
            pagePaddr = getVaddrMapedPaddr(mVaddr);
            pfree(pagePaddr);
            pageTablePteRemove(mVaddr);
            count++;
        }
        vaddrPagesRemove(pf, (uint32)vaddr, pageCount);
    }else{
        // 内核内存池
        mVaddr -= PAGE_SIZE;
        while (count < pageCount)
        {
            mVaddr += PAGE_SIZE;
            pagePaddr = getVaddrMapedPaddr(mVaddr);
            pfree(pagePaddr);
            pageTablePteRemove(mVaddr);
            count++;
        }
        vaddrPagesRemove(pf,(uint32)vaddr, pageCount);
    }
}
// 分配内核物理页
void* allocKernelPages(uint32 pageCount){
    void* vaddr = mallocPage(PF_KERNEL, pageCount);
    if(NULL != vaddr){
        memset(vaddr, 0, PAGE_SZIE);
    }
    return vaddr;
}
// 分配用户物理页
void* allocUserPages(uint32 pageCount){
    lockAcquire(&userPool.lock);
    void* vaddr = mallocPage(PF_USER, pageCount);
    memset(vaddr, 0, pageCount * PAGE_SIZE);
    lockRelease(&userPool.lock);
    return vaddr;
}
/* 关联虚拟地址与PF对应的物理页地址，仅支持1页
 * 
 */
void* mallocAPage(enum PoolFlag pf, uint32 vaddr){
    struct Pool* memPool = pf & PF_KERNEL ? &kernelPool : &userPool;
    lockAcquire(&memPool->lock);
    // 将对应的虚拟地址位图置1
    struct TaskStruct* currThread = runningThread();
    uint32 bitIndex = -1;
    if(currThread->pageDir != NULL && pf == PF_USER){
        bitIndex = (vaddr - currThread->userProgVaddrPool.vaddrStart) / PAGE_SIZE;
        ASSERT(bitIndex > 0);
        setBitmap(&currThread->userProgVaddrPool.vaddrBitmap, bitIndex, 1);
    }else if(currThread->pageDir != NULL && pf == PF_KERNEL){
        bitIndex = (vaddr - kernelVaddr.vaddrStart) / PAGE_SIZE;
        ASSERT(bitIndex > 0);
        setBitmap(&kernelVaddr.vaddrBitmap, bitIndex, 1);
    }
    void* pagePaddr = palloc(memPool);
    if(pagePaddr == NULL){
        //PANIC("mallocAPage: NET BE NULL");
        consolePrint("mallocAPage: NOT BE NULL\n");
        // 释放锁再返回
        //lockAcquire(&memPool->lock);
        return NULL;
    }
    addPageTable((void*)vaddr, pagePaddr);
    lockRelease(&memPool->lock);
    return (void*)vaddr;
}
// 获取虚拟地址（VADDR）映射的 物理地址（PADDR）
uint32 getVaddrMapedPaddr(uint32 vaddr){
    uint32* pte = getPtePtr(vaddr);
    /* (*pte)的值是页表所在的物理页框地址
     * 去掉低12位+虚拟地址的低12位
     */
    return ((*pte & 0xFFFFF000) + (vaddr & 0x00000FFF));
}
static void initMemPool(uint32 maxMemSize)
{
    printk("    *init memory pool\n");
    uint32 sizeMB = maxMemSize / 1024 / 1024;
    if(sizeMB > 1024){
        printk("Memory size Biger than 1024MiB.");
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
    memset(msgBuff, '\0', 128);
    strformat(msgBuff, "     kernel pool: PADDR_START = 0x%08x, BITMAP_VADDR = 0x%08x\n",(uint32)kernelPool.phyaddrStart, (uint32)kernelPool.bitmap.bits);
    // 输出简单的信息
    printk(msgBuff);
    memset(msgBuff, '\0', 128);
    strformat(msgBuff, "     user   pool: PADDR_START = 0x%08x, BITMAP_VADDR = 0x%08x\n",userPool.phyaddrStart, userPool.bitmap.bits);
    printk(msgBuff);
    // 初始化bitmap
    initBitmap(&kernelPool.bitmap);    
    initBitmap(&userPool.bitmap);
    //初始化内核对应的虚拟地址池
    lockInit(&kernelVaddr.lock,"KernelVaddrLock");
    kernelVaddr.vaddrStart = KERNEL_VADDR_START;
    kernelVaddr.vaddrBitmap.length = kernelPool.bitmap.length;
    kernelVaddr.vaddrBitmap.bits = (void*)KERNEL_VMEM_BITMAP_VADDR;
    //printf("     kernel vaddr: VADDR_START = 0x%x, BITMAP_PADDR = 0x%x\n", kernelVaddr.vaddrStart, kernelVaddr.vaddrBitmap.bits);
    // 初始化锁
    lockInit(&kernelPool.lock, "KernelMemLock");
    lockInit(&userPool.lock, "UserMemLock");
}

void initMem(int step)
{
    printkf("[%02d] init memory\n", step);
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
       memset(msgBuff, '\0', 128);
       strformat(msgBuff, "     BASE = 0x%s%s, LENGHT = 0x%s%s, TYPE = %d\n", bh, bl, lh, ll, ards[index].type);
       printk(msgBuff);
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
    // 初始化内存块描述符数组
    memBlockDescInit(kernelMemBlockDescs);
}
