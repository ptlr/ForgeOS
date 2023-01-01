#include "syscall.h"

/* 无参数系统调用*/

#define _syscall0(NUMBER) ({\
    int retval; \
    asm volatile(\
    "int $0x80" \
    : "=a" (retval) \
    : "a" (NUMBER) \
    : "memory"\
    );  \
    retval;\
})

#define _syscall1(NUMBER, ARG1) ({ \
    int retval;\
    asm volatile(\
    "int $0x80" \
    : "=a" (retval) \
    : "a" (NUMBER), "b" (ARG1) \
    : "memory" \
    ); \
    retval;\
})

#define _syscall2(NUMBER, ARG1, ARG2) ({ \
    int retval;\
    asm volatile(\
    "int $0x80" \
    : "=a" (retval) \
    : "a" (NUMBER), "b" (ARG1), "c"(ARG2) \
    : "memory" \
    ); \
    retval;\
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({ \
    int retval;\
    asm volatile(\
    "int $0x80" \
    : "=a" (retval) \
    : "a" (NUMBER), "b" (ARG1), "c"(ARG2), "d"(ARG3) \
    : "memory" \
    ); \
    retval;\
})

/* 返回当前任务PID */
uint32 getpid(){
    return _syscall0(SYS_GETPID);
}

uint32 write(int32 fd, char* str, uint32 count){
    return _syscall3(SYS_WRITE, fd, str, count);
}
// 申请size字节的内存
void* malloc(uint32 size){
    return (void*)_syscall1(SYS_MALLOC, size);
}
// 释放指针指向的内存
void free(void* ptr){
    _syscall1(SYS_FREE, ptr);
}
// 创建子进程
int16 fork(void){
    return _syscall0(SYS_FORK);
}
// 读取count个字节到buffer中
int32 read(int32 fd, void* buffer, uint32 count){
    return _syscall3(SYS_READ, fd, buffer, count);
}
// 显示一个字符
void putchar(char ch){
    _syscall1(SYS_PUTCHAR, ch);
}
// 清空屏幕
void clear(void){
    _syscall0(SYS_CLEAR);
}
// 获取当前工作目录
char* getcwd(char* buffer, uint32 size){
    return (char*)_syscall2(SYS_GETCWD, buffer, size);
}
// 以flag方式打开文件pathName
int32 open(char* pathName, uint8 flag){
    return _syscall2(SYS_OPEN, pathName, flag);
}
// 关闭文件
int32 close(int32 fd){
    return _syscall1(SYS_CLOSE, fd);
}
// 删除文件path
int32 unlink(const char* path){
    return _syscall1(SYS_UNLINK, path);
}
// 创建目录
int32 mkdir(const char* path){
    return _syscall1(SYS_MKDIR, path);
}
// 打开目录
struct Dir* opendir(const char* name){
    return (struct Dir*)_syscall1(SYS_OPENDIR, name);
}
// 关闭目录dir
int32 closedir(struct Dir* dir){
    return _syscall1(SYS_CLOSEDIR, dir);
}
// 删除目录
int32 rmdir(const char* name){
    return _syscall1(SYS_RMDIR, name);
}
// 读取目录
struct DirEntry* readdir(struct Dir* dir){
    return (struct DirEntry*)_syscall1(SYS_READDIR, dir);
}
// 重置目录指针
void rewinddir(struct Dir* dir){
    _syscall1(SYS_REWINDDIR, dir);
}
// 获取文件属性
int32 stat(const char* path, struct Status* status){
    return _syscall2(SYS_STAT, path, status);
}
// 改变工作目录path
int32 chdir(const char* path){
    return _syscall1(SYS_CHDIR, path);
}
// 显示任务列表
void ps(void){
    _syscall0(SYS_PS);
}
// 运行程序
int32 exec(const char* path, char* argv[]){
    return _syscall2(SYS_EXEC, path, argv);
}
// 以毫秒为单位休眠一段时间
void sleep(uint32 timeMs){
    _syscall1(SYS_SLEEP, timeMs);
}