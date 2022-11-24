#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H
#include "stdint.h"
#include "fs.h"
enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_READ,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYS_GETCWD,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_CHDIR,
    SYS_RMDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
};
// 获取进程PID
uint32 getpid(void);
// 向控制台输出字符串
uint32 write(int32 fd, char* str, uint32 count);
// 申请size字节的内存
void* malloc(uint32 size);
// 释放指针指向的内存
void free(void* ptr);
// 创建子进程
int16 fork(void);
// 读取count个字节到buffer中
int32 read(int32 fd, void* buffer, uint32 count);
// 显示一个字符
void putchar(char ch);
// 清空屏幕
void clsScreen(void);
// 获取当前工作目录
char* getcwd(char* buffer, uint32 size);
// 以flag方式打开文件pathName
int32 open(char* pathName, uint8 flag);
// 关闭文件
int32 close(int32 fd);
// 删除文件path
int32 unlink(const char* path);
// 创建目录
int32 mkdir(const char* path);
// 打开目录
struct Dir* opendir(const char* name);
// 关闭目录dir
int32 closedir(struct Dir* dir);
// 删除目录
int32 rmdir(const char* name);
// 读取目录
struct DirEntry* readdir(struct Dir* dir);
// 重置目录指针
void rewinddir(struct Dir* dir);
// 获取文件属性
int32 stat(const char* path, struct Status* status);
// 改变工作目录path
int32 chdir(const char* path);
// 显示任务列表
void ps(void);
#endif