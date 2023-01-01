#ifndef LIB_USER_ASSERT_H
#define LIB_USER_ASSERT_H
/* 用于条件不满足时输出调试信息
 */
void userSpin(char* fileName, int line, const char* func, const char* condition);

/* 用#define指令把PANIC对应的函数指向panicSpin
 * __FILE__, __LINE__, __func__, __VA_ARGS__ 是由编译器提供的表示符，
 * 分别表示哪个文件，文件的第几行，在哪个函数里，以及可变长度参数，也可以用"..."表示
 */
#define painc(...) userSpin(__FILE__, __LINE__, __func__, __VA_ARGS__)

/*利用参数编译时的参数开关来决定是否启用DEBUG*/
#ifdef NDEBUG
    /* 编译时启用了NDEBUG宏，则把ASSERT定义成空，相当于删除了ASSERT函数 */
    #define assert(CONDITION) ((void)0)
#else
    #define assert(CONDITION) if(CONDITION){}else{painc(#CONDITION);}
#endif /*NDEBUG*/
#endif