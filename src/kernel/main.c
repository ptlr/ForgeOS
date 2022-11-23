#include "init.h"
#include "debug.h"
#include "printk.h"
#include "stdint.h"
#include "interrupt.h"
#include "string.h"
#include "stdio.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"
#include "process.h"
#include "bitmap.h"
#include "syscall.h"
#include "syscall-init.h"
#include "stdint.h"
#include "fs.h"
#include "file.h"
extern void kernelMain(void);
char* MSG_KERNEL = "[05] kernel start\n";
char* firstFile = "/first";
char* subDir = "/dir1";
char* dir1 = "/dir1/subdir1";
char* file2 = "/dir1/subdir1/file2";
char buff[128];
uint16 UPA_PID;
uint16 UPB_PID = 0x64;
void kThreadA(void*);
void kThreadB(void*);
void uProcA(void);
void uProcB(void);
void kernelMain(void)
{
    printkf("\n\n\n\n%s", MSG_KERNEL);
    init();
    intrEnable();
    /*processExecute(uProcA, "UPA");
    processExecute(uProcB, "UPB");
    startThread("KTA", 31, kThreadA, "KTA");
    startThread("KTB", 31, kThreadB, "KTB");*/
    //cfile();
    //printf("'%s' delete %s!\n", firstFile, sysUnlink(firstFile) == 0 ? "done" : "failed");
    //mkdir();
    //opendir();
    //rmdirTest();
    //cwdTest();
    fileInfoTest();
    while(1);
}
// 文件信息测试
void fileInfoTest(void){
    struct Status objStat;
    sysStat("/", &objStat);
    printf("'%s' info:\n    inodeNum:%4d\n    size:%4d\n    FileType:%s\n", "/",\
    objStat.inodeNum, objStat.size, objStat.fileType == FT_FILE ? "FILE" : "DIR ");
    sysStat("/dir1", &objStat);
    printf("'%s' info:\n    inodeNum:%4d\n    size:%4d\n    FileType:%s\n", "/dir1",\
    objStat.inodeNum, objStat.size, objStat.fileType == FT_FILE ? "FILE" : "DIR ");
}
// 工作区切换测试
void cwdTest(void){
    char cwdBuff[32] = {0};
    sysGetCwd(cwdBuff, 32);
    printf("CWD: %s\n", cwdBuff);
    sysChdir("/dir1");
    printf("change CWD now\n");
    sysGetCwd(cwdBuff, 32);
    printf("CWD: %s\n", cwdBuff);
}
void cfile(void){
    uint32 fd = sysOpen(firstFile, O_CREAT);
    sysClose(fd);

}
void ofile(void){
    uint32 fd = sysOpen(firstFile, O_RDWR);
}
void mkdir(void){
    printf("[MKDIR01] '%s' creat %s!\n",dir1, sysMkdir(dir1) == 0 ? "done" : "failed");
    printf("[MKDIR02] '/dir1' creat %s!\n", sysMkdir("/dir1") == 0 ? "done" : "failed");
    printf("[MKDIR03] '%s' creat %s!\n",dir1, sysMkdir(dir1) == 0 ? "done" : "failed");
    int fd = sysOpen(file2, O_CREAT | O_RDWR);
    if(fd != -1){
        printf("[MKDIR04] '%s' creat done!\n", file2);
        sysWrite(fd, "yeah! i can fly!\n",18);
        sysLSeek(fd, 0, SEEK_SET);
        char msg[32] = {0};
        sysRead(fd, msg, 18);
        printf("[MKDIR04] '%s' says: \n%s", file2, msg);
        sysClose(fd);
    }
}
void opendir(void){
    struct Dir* parentDir = sysOpendir(dir1);
    if(parentDir){
        printf("'%s' open done!\ncontent:\n", dir1);
        char* type = NULL;
        struct DirEntry* dirEntryPtr = NULL;
        while (dirEntryPtr = sysReadDirEntry(parentDir))
        {
            if(dirEntryPtr->fileType == FT_FILE){
                type = "FILE";
            }else{
                type = "DIR ";
            }
            printf("%s %s\n", type, dirEntryPtr->fileName);
        }
        if(sysCloseDir(parentDir) == 0){
            printf("'%s' close done!\n", dir1);
        }else{
            printf("'%s' close faild!\n", dir1);
        }
        
    }else{
        printf("'%s' open failed!\n", dir1);
    }
}
// 删除目录测试
void rmdirTest(void){
    printf("'%s' content before delete '%s':\n", subDir, dir1);
    struct Dir* dirPtr = sysOpendir(subDir);
    char* type = NULL;
    struct DirEntry* dirEntry = NULL;
    while(dirEntry = sysReadDirEntry(dirPtr)){
        if(dirEntry->fileType == FT_FILE){
            type = "FILE";
        }else{
            type = "DIR ";
        }
        printf("%s %s\n", type, dirEntry->fileName);
    }
    printf("try to delete nonempty dir '%s'\n", dir1);
    printf("'%s' delete %s!\n", dir1, sysRmdir(dir1) == -1 ? "failed" : "done");
    printf("try to delete '%s'\n", file2);
    printf("'%s' delete %s\n", file2, sysUnlink(file2) == -1 ? "failed":"done");
    printf("try to delete nonempty dir '%s' again\n", dir1);
    printf("'%s' delete %s!\n", dir1, sysRmdir(dir1) == -1 ? "failed" : "done");

    printf("'%s' content after delete '%s':\n", subDir, dir1);
    sysRewinddir(dirPtr);
    while((dirEntry = sysReadDirEntry(dirPtr))){
        if(dirEntry->fileType == FT_FILE){
            type = "FILE";
        }else{
            type = "DIR ";
        }
        printf("%s %s\n", type, dirEntry->fileName);
    }
}
void kThreadA(void* arg){
    char buffer[256];
    memset(buff,'\0', 256);
    void* vaddr1 = sys_malloc(256);
    void* vaddr2 = sys_malloc(255);
    void* vaddr3 = sys_malloc(254);
    strformat(buffer, "KTA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    consolePrint(buffer);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    sys_free(vaddr1);
    sys_free(vaddr2);
    sys_free(vaddr3);
    while(1);
}
void kThreadB(void* arg){
    char buffer[256];
    memset(buff,'\0', 256);
    void* vaddr1 = sys_malloc(256);
    void* vaddr2 = sys_malloc(255);
    void* vaddr3 = sys_malloc(254);
    strformat(buffer, "KTA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    consolePrint(buffer);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    sys_free(vaddr1);
    sys_free(vaddr2);
    sys_free(vaddr3);
    while(1);
}
void uProcA(void){
    void* vaddr1 = malloc(256);
    void* vaddr2 = malloc(255);
    void* vaddr3 = malloc(254);
    printf("UPA VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    free(vaddr1);
    free(vaddr2);
    free(vaddr3);
    while(1);
}
void uProcB(void){
    void* vaddr1 = malloc(256);
    void* vaddr2 = malloc(255);
    void* vaddr3 = malloc(254);
    printf("UPB VADDR: 0x%08x,0x%08x,0x%08x\n", (uint32)vaddr1, (uint32)vaddr2, (uint32)vaddr3);
    int cpu_delay = 10000;
    while (cpu_delay-- > 0);
    free(vaddr1);
    free(vaddr2);
    free(vaddr3);
    while(1);
}