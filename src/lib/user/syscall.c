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
void clsScreen(void){
    _syscall0(SYS_CLEAR);
}