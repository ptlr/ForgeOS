#include "exec.h"
#include "constant.h"
#include "memory.h"
#include "fs.h"
#include "string.h"
#include "thread.h"
#include "printk.h"
#include "debug.h"

// 将文件描述符fd指向的文件中，偏移为offset，大小为fileSize的段加载到地址为vaddr的内存中
static bool segmentLoad(int32 fd, uint32 offset, uint32 fileSize, uint32 vaddr){
    // 计算出程序段的第一个页地址
    uint32 firstPageVaddr = vaddr & 0xFFFFF000;
    // 计算需要加载的程序在第一个页框内占了多少字节
    // 说明：vaddr & 0x00000FFF计算出，数据在页内的偏移地址，页大小减去偏移地址得出占用大小
    uint32 firstPageUsedCnt = PAGE_SIZE - (vaddr & 0x00000FFF);
    uint32 occupyPages = 1;
    if(fileSize > firstPageUsedCnt){
        uint32 leftSize = fileSize - firstPageUsedCnt;
        // 追加剩下大小需要的页数
        occupyPages += DIV_ROUND_UP(leftSize, PAGE_SIZE);
    }
    uint32 pageIndex = 0;
    uint32 pageVaddr = firstPageVaddr;
    while(pageIndex < occupyPages){
        //printkf("PI=%4d,OP=%4d\n", pageIndex, occupyPages);
        uint32* pde = getPdePtr(pageVaddr);
        uint32* pte = getPtePtr(pageVaddr);
        /* 如果pde或pte不存在就分配内存
         * 注意：pde判断应该先在前，否则，当pde不存在时，判断pte会发生缺页异常
         */
        if(!(*pde & 0x00000001) || !(*pte & 0x00000001)){
            printk("not exist!!!!\n");
            if(mallocAPage(PF_USER, pageVaddr) == NULL){
                return false;
            }
        }else{
            printkf("exist!!!!\nPDE-VADDR:0x%08x, PTE-VADDR:0x%08x\n", pde, pte);
        }
        // 如果存在，则利用旧的物理页
        pageVaddr += PAGE_SZIE;
        pageIndex++;
    }
    sysLSeek(fd, offset, SEEK_SET);
    sysRead(fd, (void*)vaddr, fileSize);
    return true;
}
// 从文件中加载用户程序, 成功返回起始地址，失败返回-1
static int32 load(const char* path){
    int32 retVal = -1;
    uint32 elfHeaderSize = sizeof(struct Elf32Ehdr);
    struct Elf32Ehdr elfHeader;
    struct Elf32Phdr progHeader;
    memset(&elfHeader, 0, elfHeaderSize);
    printkf("Try open file! FileName=%s\n",path);
    int32 fd = sysOpen(path, O_RDONLY);
    if(fd == -1) return retVal;
    printkf("File opened!FileFD=%d\n", fd);
    if(sysRead(fd, &elfHeader, elfHeaderSize) != elfHeaderSize){
        printk("ERROR!\n");
        retVal = -1;
        goto done;
    }
    // 校验elf头
    if(memcmp(elfHeader.eIndent,"\177ELF\1\1\1", 7) \
    || elfHeader.eType != 2 \
    || elfHeader.eMachine != 3 \
    || elfHeader.eVersion != 1 \
    || elfHeader.ePhEntryCnt > 1024 \
    || elfHeader.ePhEntrySize != sizeof(struct Elf32Phdr)){
        printkf("ELF32 CHK: Type=%d, Machine=%d, Version=%d, phENtryCnt=%d, phEntrySize=%d\n", elfHeader.eType, elfHeader.eMachine, elfHeader.eVersion, \
        elfHeader.ePhEntryCnt, elfHeader.ePhEntrySize);
        retVal = -1;
        goto done;
    }
    ELF32_OFF progHeadOff = elfHeader.ePhOff;
    ELF32_HALF progHeadSize = elfHeader.ePhEntrySize;
    uint32 progIndex = 0;
    // 遍历所有程序头
    while(progIndex < elfHeader.ePhEntryCnt){
        printkf("CNT=%d,PI=%d\n", elfHeader.ePhEntryCnt, progIndex);
        memset(&progHeader, 0, progHeadSize);
        sysLSeek(fd, progHeadOff, SEEK_SET);
        // 只读取程序头
        if(sysRead(fd, &progHeader, progHeadSize) != progHeadSize){
            retVal = -1;
            goto done;
        }
        printk("LOOP LOAD START!\n");
        // 找到可用段加载至内存
        if(PT_LOAD == progHeader.pType){
            if(!segmentLoad(fd, progHeader.pOffset, progHeader.pFileSize, progHeader.pVaddr)){
                retVal = -1;
                goto done;
            }
        }
        // 更新下一个程序头偏移
        progHeadOff += elfHeader.ePhEntrySize;
        progIndex++;
    }
    printk("LOOP END!\n");
    retVal = elfHeader.eVaddrEntry;
done:
    sysClose(fd);
    return retVal;
}
// 用path指向的程序代替当前线程
int32 sysExecv(const char* path, const char* argv[]){
    uint32 argc = 0;
    while(argv[argc]){
        argc++;
    }
    int32 entryPoint = load(path);
    if(-1 == entryPoint){
        printk("ENTRY_PORINT ERROR!\n");
        return -1;
    }
    printk("LOAD FINISH!\n");
    struct TaskStruct* curentThread = runningThread();
    // 修改进程名
    memcpy(curentThread->name, path, TASK_NAME_LEN);
    curentThread->name[TASK_NAME_LEN - 1] = 0;
    struct IntrStack* intr0Stack = (struct IntrStack*) ((uint32)curentThread + PAGE_SIZE - sizeof(struct IntrStack));
    // 将参数传递给用户进程
    intr0Stack->EBX = (int32)argv;
    intr0Stack->ECX = argc;
    intr0Stack->EIP = (void*)entryPoint;
    intr0Stack->ESP = (void*)0xC0000000;
    printk("START USER PROG\n");
    // 为了让进程更快执行，直接从中断返回
    asm volatile("movl %0, %%esp; jmp intrExit"::"g" (intr0Stack):"memory");
    return 0;
}