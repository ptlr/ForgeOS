#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H
#include "stdint.h"
/* 用于条件不满足时输出调试信息
 */
void panicSpin(char* fileName, int line, const char* func, const char* condition);

/* 用#define指令把PANIC对应的函数指向panicSpin
 * __FILE__, __LINE__, __func__, __VA_ARGS__ 是由编译器提供的表示符，
 * 分别表示哪个文件，文件的第几行，在哪个函数里，以及可变长度参数，也可以用"..."表示
 */
#define PANIC(...) panicSpin(__FILE__, __LINE__, __func__, __VA_ARGS__)

/*利用参数编译时的参数开关来决定是否启用DEBUG*/
#ifdef NDEBUG
    /* 编译时启用了NDEBUG宏，则把ASSERT定义成空，相当于删除了ASSERT函数 */
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) if(CONDITION){}else{PANIC(#CONDITION);}
#endif /*NDEBUG*/
void log(const char* label, const char* msg, uint8 color);
void logInfor(const char* msg);
void logWaring( const char* msg);
void logError( const char* msg);
#endif /*KERNEL_DEBUG_H*/