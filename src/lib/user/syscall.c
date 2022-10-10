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
    : "a" (NUMBER), "b" (ARGa), "c"(ARG2) \
    : "memory" \
    ); \
    retval;\
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({ \
    int retval;\
    asm volatile(\
    "int $0x80" \
    : "=a" (retval) \
    : "a" (NUMBER), "b" (ARG), "c"(ARG2), "d"(ARG3) \
    : "memory" \
    ); \
    retval;\
})

/* 返回当前任务PID */
uint32 getpid(){
    return _syscall0(SYS_GETPID);
}

uint32 write(char* str){
    return _syscall1(SYS_WRITE, str);
}
// 申请size字节的内存
void* malloc(uint32 size){
    return (void*)_syscall1(SYS_MALLOC, size);
}
// 释放指针指向的内存
void free(void* ptr){
    return _syscall1(SYS_FREE, ptr);
}