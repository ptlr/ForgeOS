#include "ide.h"
#include "printk.h"
#include "string.h"
#include "io.h"
#include "debug.h"
#include "timer.h"
#include "interrupt.h"
#include "memory.h"
#include "list.h"

// 硬盘操作指令
#define CMD_IDENTIFY        0xEC    // identify指令
#define CMD_READ_SECTOR     0x20    // 读扇区
#define CMD_WRITE_SECOR     0x10    // 写扇区
// ToDo: 调试后移除， 定义可读写的最大扇区数(128MiB)
#define MAX_LBA ((128 * 1024 *1024 / 512) - 1)
// 寄存器关键位
#define BIT_ALT_STAT_BSY    0x80
#define BIT_ALT_STAT_DRDY   0x40
#define BIT_ALT_STAT_DRQ    0x08
#define BIT_DEV_MBS         0xA0
#define BIT_DEV_LBA         0x40
#define BIT_DEV_DEV         0x10
// 硬盘寄存器的端口号
#define REG_DATA(channel)       (channel->basePort + 0)
#define REG_ERROR(channel)      (channel->basePort + 1)
#define REG_SECTOR_CNT(channel) (channel->basePort + 2)
#define REG_LBA_L(channel)      (channel->basePort + 3)
#define REG_LBA_M(channel)      (channel->basePort + 4)
#define REG_LBA_H(channel)      (channel->basePort + 5)
#define REG_DEV(channel)        (channel->basePort + 6)
#define REG_STATUS(channel)     (channel->basePort + 7)
#define REG_CMD(channel)        (REG_STATUS(channel))
#define REG_ALT_STATUS(channel) (channel->basePort + 0x206)
#define REG_CTL(channel)        (REG_ALT_STATUS(channel))
// 总扩展分区的起始LBA, 初始化为0， 扫描分区时以此作为标记
int32 extendLbaBase = 0;
// 记录主分区和逻辑分区的下标
uint8 primaryPartIndex = 0;
uint8 logicPartIndex = 0;
// 分区队列
struct List partitionLists;
// 硬盘通道数
uint8 channelCount;
// IDE通道
struct IdeChannel channels[2];
// 数据结构
// 分区表项
struct PartitionTableEntry{
    uint8 bootable;         // 是否可引导
    uint8 startHead;        // 起始磁头号
    uint8 startSector;      // 起始扇区号
    uint8 startCHS;         // 起始柱面
    uint8 fsType;           // 分区类型
    uint8 endHead;          // 结束磁头号
    uint8 endSec;           // 结束扇区
    uint8 endCHS;           // 结束柱面
    uint32 startLBA;        // 本分区起始扇区的LBA地址
    uint32 sectorCount;     // 本分区的扇区数目
} __attribute__ ((packed));      // packed属性保证此结构是16字节大小
// 启动扇区
struct BootSector{
    uint8 code[446];
    struct PartitionTableEntry partitionTable[4];
    uint16 signature;
}__attribute__((packed));
// 将dst中len个相邻的字节交换位置后存入buf
static void swapPairsBytes(const char* dst, char* buf, uint32 len){
    uint8 index;
    for(index = 0; index < len; index += 2){
        buf[index + 1] = *dst++;
        buf[index]     = *dst++;
    }
    buf[index] = '\0';
}
// 向通道发送命令
static void cmdOut(struct IdeChannel* channel, uint8 cmd){
    // 向硬盘发送命令后标记为true，等待中断处理
    channel->waitingIrq =  true;
    outb(REG_CMD(channel), cmd);
}
// 选择要操作的硬盘
static void selectDisk(struct Disk* disk){
    uint8 regBits = BIT_DEV_MBS | BIT_DEV_LBA;
    if(disk->devNum == 1) regBits |= BIT_DEV_DEV;
    outb(REG_DEV(disk->channel), regBits);
}
/* 等待30秒 */
static bool busyWait(struct Disk* disk){
    struct IdeChannel* channel = disk->channel;
    // 机械硬盘在极端情况下会在32秒内完成读写操作（ATA手册）
    uint16 timeLimitMs = 30 * 1000;
    while (timeLimitMs > 0)
    {
        /* 警告： 这里由于括号导致读取状态的时候出错，无法完成busyWait
         */
        if(!(inb(REG_STATUS(channel)) & BIT_ALT_STAT_BSY)){
            return inb(REG_STATUS(channel)) & BIT_ALT_STAT_DRQ;
        }else{
            //printk("Busy Wait >> sleep!\n");
            msSleep(10);
            timeLimitMs -= 10;
        }  
    }
    return false;
}
// 选择要操作的扇区
static void selectSector(struct Disk* disk, uint32 lba, uint8 sectorCount){
    ASSERT(lba < MAX_LBA);
    struct IdeChannel* channel = disk->channel;
    // 写入要读取的扇区数, 0 表示读取256个扇区
    outb(REG_SECTOR_CNT(channel), sectorCount);
    // 写入LBA
    outb(REG_LBA_L(channel), lba);
    outb(REG_LBA_M(channel), lba >> 8);
    outb(REG_LBA_H(channel), lba >> 16);
    // 重写device寄存器，原因：lba24~27位需要写到该寄存器的0~3位
    outb(REG_DEV(channel), BIT_DEV_MBS | BIT_DEV_LBA | (disk->devNum == 1 ? BIT_DEV_DEV : 0) | (lba >> 24));
}
// 读取操作
static void readSectors(struct Disk* disk, void* buf, uint8 sectorCount){
    uint32 sizeByte;
    if(sectorCount == 0){
        sizeByte = 256 * 512;
    }else{
        sizeByte = sectorCount * 512;
    }
    insw(REG_DATA(disk->channel), buf, sizeByte / 2);
}
// 写入操作
static void writeSectors(struct Disk* disk, void* buf, uint8 sectorCount){
    uint32 sizeByte;
    if(sectorCount == 0){
        sizeByte = 256 * 512;
    }else{
        sizeByte = sectorCount * 512;
    }
    outsw(REG_DATA(disk->channel), buf, sizeByte / 2);
}
// 读取sectorCount个扇区到buff
void ideRead(struct Disk* disk, uint32 lba, void* buf, uint32 sectorCount){
    ASSERT(lba <= MAX_LBA);
    ASSERT(sectorCount > 0);
    lockAcquire(&disk->channel->lock);
    selectDisk(disk);
    // 当前操作的扇区数
    uint32 secsOpCount;
    // 已经写完的任务数
    uint32 secsDoneCount = 0;
    while(secsDoneCount < sectorCount){
        if((secsDoneCount + 256) <= sectorCount){
            secsOpCount = 256;
        }else{
            secsOpCount = sectorCount - secsDoneCount;
        }
        selectSector(disk, lba + secsDoneCount, secsOpCount);
        cmdOut(disk->channel, CMD_READ_SECTOR); // 准备开始读取数据
        semaDown(&disk->channel->seamphore);
        if(!busyWait(disk)){
            char error[64];
            strformat(error, "%s read sector %d failed!!!!!\n", disk->name, lba);
            PANIC(error);
        }

        readSectors(disk, (void*)((uint32)buf + secsDoneCount * 512), secsOpCount);
        secsDoneCount += secsOpCount;
    }
    lockRelease(&disk->channel->lock);
}
// 写将buf中sectorCount扇区数写入硬盘
void ideWrite(struct Disk* disk, uint32 lba, void* buf, uint32 sectorCount){
    ASSERT(lba <= MAX_LBA);
    ASSERT(sectorCount > 0);
    lockAcquire(&disk->channel->lock);
    // 当前操作的扇区数
    uint32 secsOpCount;
    // 已经写完的任务数
    uint32 secsDoneCount = 0;
    while(secsDoneCount < sectorCount){
        if((secsDoneCount + 256) <= sectorCount){
            secsOpCount = 256;
        }else{
            secsOpCount = sectorCount - secsDoneCount;
        }
        selectSector(disk, lba + secsDoneCount, secsOpCount);
        cmdOut(disk->channel, CMD_WRITE_SECOR); // 准备开始读取数据
        semaDown(&disk->channel->seamphore);
        if(!busyWait(disk)){
            char error[64];
            strformat(error, "%s write sector %d failed!!!!!\n", disk->name, lba);
            PANIC(error);
        }

        writeSectors(disk, (void*)((uint32)buf + secsDoneCount * 512), secsOpCount);
        secsDoneCount += secsOpCount;
    }
    lockRelease(&disk->channel->lock);

}
// 获取硬盘参数
static void identifyDisk(struct Disk* disk){
    char idInfo[512];
    selectDisk(disk);
    cmdOut(disk->channel, CMD_IDENTIFY);
    /* 向硬盘发送指令后通过信号量阻塞自己， 等硬盘处理完成后通过硬盘唤醒自己 */
    semaDown(&disk->channel->seamphore);
    //printk("BBW\n");
    /* 唤醒后执行 */
    if(!busyWait(disk)){
        char error[64];
        strformat(error, "%s identify failed!\n", disk->name);
        PANIC(error);
    }
    //printk("ABW\n");
    readSectors(disk, idInfo, 1);
    char buff[64];
    uint8 snStart = 10 * 2;
    uint8 snLen = 20;
    uint8 moduleStart = 27 * 2;
    uint8 moduleLen = 40;
    swapPairsBytes(&idInfo[snStart], buff, snLen);
    printkf("Disk info(%s):\n    SN      : %s\n", disk->name, buff);
    memset(buff, '\0', sizeof(buff));
    swapPairsBytes(&idInfo[moduleStart], buff, moduleLen);
    printkf("    MODULE  : %s\n", buff);
    uint32 sectors = *(uint32*)&idInfo[60 * 2];
    printkf("    SECTORS : %d\n    CAPACITY: %dMiB\n", sectors, sectors * 512 / 1024 /1024);
}
// 扫描硬盘disk中地址为extendLba的扇区中的所有分区
static void partitionScan(struct Disk* disk, uint32 extendLba){
    struct BootSector* bs = sys_malloc(sizeof(struct BootSector));
    ideRead(disk, extendLba, bs, 1);
    uint8 partIndex = 0;
    struct PartitionTableEntry* partitionTableEntry = bs->partitionTable;
    while(partIndex++ < 4){
        if(partitionTableEntry->fsType == 0x05){
            //printk("Find extend part!!!\n");
            // 扩展分区
            if(extendLbaBase != 0){
                // 子扩展分区的startLba相对与主引导扇区中的总扩展分区地址
                partitionScan(disk, partitionTableEntry->startLBA + extendLbaBase);
            }else{
                // extendLbaBase = 0，表示第一次读取引导块, 记录下来
                //printk("First!!!!!!!!!!!!\n");
                extendLbaBase = partitionTableEntry->startLBA;
                partitionScan(disk, partitionTableEntry->startLBA);
            }
        }else if(partitionTableEntry->fsType != 0x00){
            //printk("Find prim part!!!\n");
            // 如果是有效的分区类型
            if(extendLba == 0){ // 表示全部是主分区
                disk->primaryParts[primaryPartIndex].startLBA = extendLba + partitionTableEntry->startLBA;
                disk->primaryParts[primaryPartIndex].sectorCount = partitionTableEntry->sectorCount;
                disk->primaryParts[primaryPartIndex].disk = disk;
                listAppend(&partitionLists, &disk->primaryParts[primaryPartIndex].partTag);
                strformat(disk->primaryParts[primaryPartIndex].name, "%s%d", disk->name, primaryPartIndex + 1);
                primaryPartIndex++;
            }else{
                // 逻辑分区
                disk->logicParts[logicPartIndex].startLBA =  extendLba + partitionTableEntry->startLBA;
                disk->logicParts[logicPartIndex].sectorCount = partitionTableEntry->sectorCount;
                disk->logicParts[logicPartIndex].disk = disk;
                listAppend(&partitionLists, &disk->logicParts[logicPartIndex].partTag);
                // 从5开始
                strformat(disk->logicParts[logicPartIndex].name, "%s%d", disk->name, logicPartIndex + 5);
                logicPartIndex++;
                ASSERT(logicPartIndex < 8);
                // 目前只支持8个逻辑分区
                if(logicPartIndex >= 8) return;
            }
        }
        partitionTableEntry++;
    }
    sys_free(bs);
}
static bool partitionInfo(struct ListElem* pelem, int arg){
    struct Partition* part = elem2entry(struct Partition, partTag, pelem);
    printkf("    %s: StartLBA=%6d, SectorCount=%6d, Capacity=%6dMiB\n", part->name, part->startLBA, part->sectorCount ,((uint32)part->sectorCount * 512) / 1024 / 1024);
    // 返回false让listTraversal遍历至末尾
    return false;
}

void intrDiskHandler(uint8 irqNum){
    ASSERT(irqNum == 0x2E || irqNum == 0x2F);
    uint8 channelNum = irqNum - 0x2E;
    struct IdeChannel* channel = &channels[channelNum];
    ASSERT(channel->irqNum == irqNum);
    /* 每次写硬盘时会申请锁， 从而保证一致性 */
    if(channel->waitingIrq){
        channel->waitingIrq = false;
        semaUp(&channel->seamphore);
        /*uint8 statusBits =  inb(REG_STATUS(channel));
        uint8 oldColor = setColor(COLOR_BG_DARK | COLOR_FG_RED);
        //printkf("IDE IRQ_NUM = 0x%2x, STATUS = 0b%8b\n", irqNum, (uint32)statusBits);
        setColor(oldColor);*/
    }
}

void initIDE(int (* step)(void)){
    printkf("[%02d] init IDE\n", step());
    listInit(&partitionLists);
    // 地址0x475中保存着硬盘个数
    uint8 diskCount = *((uint8*)0x475);
    // 计算有几个通道
    channelCount = DIV_ROUND_UP(diskCount, 2);
    struct IdeChannel* channel;
    uint8 channelNum = 0;
    uint8 deviceNum = 0;
    while(channelNum < channelCount){
        channel = &channels[channelNum];
        strformat(channel->name, "IDE%d", channelNum);
        /* 为每个通道初始化端口基址及中断向量 */
        switch (channelNum)
        {
        case 0:
            channel->basePort = 0x1F0;      // IDE0通道的起始端口号是0x1F0
            channel->irqNum = 0x20 + 14;    // 从片上倒数第二个引脚，IDE0的中断号
            lockInit(&channel->lock, "IDE0_LOCK");
            break;
        case 1:
            channel->basePort = 0x170;      // IDE0通道的起始端口号是0x1F0
            channel->irqNum = 0x20 + 15;    // 从片上倒数第二个引脚，IDE0的中断号
            lockInit(&channel->lock, "IDE1_LOCK");
            break;
        default:
            break;
        }
        channel->waitingIrq = false;
        /* 初始化为0，目的是向硬盘控制器请求数据后，硬盘semaDown此信号阻塞线程，直到硬盘完成任务发起中断，再由中断semaUp并唤醒线程
         */
        semaInit(&channel->seamphore, 0);
        printkf("    Disk: NAME=%s, IRQ_NUM=0x%04x, BASE_PORT=0x%04x\n", channel->name, channel->irqNum, channel->basePort);
        registerHandler(channel->irqNum, intrDiskHandler);
        // 每个通道上有两块硬盘
        while(deviceNum < 2){
            //printkf("DI=%d\n",deviceNum);
            struct Disk* disk = &channel->devices[deviceNum];
            disk->channel = channel;
            disk->devNum = deviceNum;
            strformat(disk->name, "sd%c",'a' + channelNum * 2 + deviceNum);
            identifyDisk(disk);
            //printk("After identify\n");
            // 不处理内核本身所在的裸盘
            if(deviceNum != 0){
                partitionScan(disk, 0);
            }
            primaryPartIndex =0;
            logicPartIndex = 0;
            deviceNum++;
        }
        deviceNum = 0;
        channelNum++;
    }
    // 打印分区信息
    printk("    Partition info:\n");
    listTraversal(&partitionLists,  partitionInfo, (int)NULL);
}