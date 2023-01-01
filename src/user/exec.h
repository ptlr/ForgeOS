#ifndef USER_EXEC_H
#define USER_EXEC_H

#include "stdint.h"

extern void intrExit(void);

typedef uint32 ELF32_WORD, ELF32_ADDR, ELF32_OFF;
typedef uint16 ELF32_HALF;
/* 32位ELF头 */
struct Elf32Ehdr{
    unsigned char   eIndent[16];    // 魔数
    ELF32_HALF      eType;          // 文件类型
    ELF32_HALF      eMachine;       // 架构
    ELF32_WORD      eVersion;       // 文件版本
    ELF32_ADDR      eVaddrEntry;    // 虚拟地址入口
    ELF32_OFF       ePhOff;         // 程序头表偏移地址
    ELF32_OFF       eShOff;         // 节头表偏移地址
    ELF32_WORD      eFlags;         // 处理器标识
    ELF32_HALF      eEhSize;        // ELF头大小
    ELF32_HALF      ePhEntrySize;   // 程序头表项大小
    ELF32_HALF      ePhEntryCnt;    // 程序表头项数目
    ELF32_HALF      eShEntrySize;   // 节表头大小
    ELF32_HALF      eShCnt;         // 节表头项个数
    ELF32_HALF      eShstrndx;      // 节表头字符表索引
};
// 段类型
enum SegmentType{
    PT_NULL,        // 忽略
    PT_LOAD,        // 可加载程序段
    PT_DYNAMIC,     // 动态加载信息
    PT_INTERR,      // 动态加载器名称
    PT_NOTE,        // 辅助信息
    PT_SHLIB,       // 保留
    PT_PHDR,        // 程序头表
};
// 段描述头
struct Elf32Phdr{
    ELF32_WORD  pType;      // SegmentType
    ELF32_OFF   pOffset;    // 该段文件偏移
    ELF32_ADDR  pVaddr;     // 段虚拟地址
    ELF32_ADDR  pPaddr;     // 段物理地址
    ELF32_WORD  pFileSize;  // 段文件大小
    ELF32_WORD  pMemSize;   // 段文件在内存中的大小
    ELF32_WORD  pFlags;     // 段flag
    ELF32_WORD  pAlign;     // 段对齐
};
// 用path指向的程序代替当前线程
int32 sysExecv(const char* path, const char* argv[]);
#endif