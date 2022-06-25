#include "debug.h"
#include "string.h"
#include "constant.h"
#include "../include/stdint.h"
#include "stdio.h"
//内存相关

/* 功能：将从dst_开始size个字节设置为value
 * !注意：由这个函数引起一般性保护异常时，请检查调用函数的地址是否由问题
 */
void memset(void* dst_, uint8 value, uint32 size)
{
    ASSERT(dst_ != NULL);
    uint8* dst = (uint8*)dst_;
    for(uint32 index = 0; index < size; index++)
    {
        *dst++ = value;
    }
}
/* 功能： 将src_开始的size个字节复制到dst_
 */
void memcpy(void* dst_, void* src_, uint32 size)
{
    ASSERT(dst_!= NULL && src_ != NULL);
    uint8* dst = (uint8*)dst_;
    const uint8* src = (uint8*) src_;
    while (size-- > 0)
    {
        *dst++ = *src++;
    }  
}

/* 功能： 连续比较a_和b_开头的开头的size个字节
 * 返回： 如果都相同返回0,若a_ > b_返回1，否则返回-1
 */
int memcmp(void* a_, void* b_, uint32 size)
{
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL && b != NULL);
    while (size-- > 0){
        if(*a != *b){
            return *a > *b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

// 字符串相关
/* 功能：将字符串从src_复制到dst_
 */
char* strcpy(char* dst_, const char* src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char* r = dst_;
    /*
     *
     */
    while ((*dst_++ = *src_++));
    return r;
}

/* 功能： 将src_拼接到dst_后, 返回拼接后的地址
 */
char* strcat(char* dst_, const char* src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char* str = dst_;
    // 定位至dst_最后一个位置
    while (*str++);
    //ToDo: 这里可能会存在问题
    str--;
    while((*str++ = *src_++));
    return dst_;
}

/* 功能： 返回字符串的长度
 */
uint32 strlen(const char* str)
{
    ASSERT(str != NULL);
    /* 原理: 字符串以0结尾，用*p++遍历时，会到*p = 0时，结束循环
     * 返回：每个字符占用一个字节，利用地址相减再减1就是长度
     */
    const char* p = str;
    while(*p++);
    return (p - str - 1);
}

/* 功能：比较两个字符串
 * 返回：a > b时返回1, 相等时返回0, 否则返回-1
 */
int8 strcmp(const char* a, const char* b)
{
    ASSERT(a != NULL && b != NULL);
    while (*a != 0 && *a == *b)
    {
        a++;
        b++;
    }
    /* *a < *b 时-1。
     * 否则*a > *b 有两种情况，成立时是1(TRUE),不成立时是0（FALSE）
     */
    return *a < *b ? -1 : *a > *b;
    
}

/* 功能： 从左到右查找字符串str中首次出现ch的地址
 */
char* strchr(const char* str, const uint8 ch)
{
    ASSERT(str != NULL);
    while (*str != 0)
    {
        if(*str == ch) return (char*)str;
        str++;
    }
    return NULL;
}

/* 功能： 从后往前查找字符串str中首次出现ch的地址
 */
char* strrchr(const char* str, const uint8 ch)
{
    ASSERT(str != NULL);
    const char* lastChar = NULL;
    while (*str != 0)
    {
        if(*str == ch){
            lastChar = str;
        }
        str++;
    }
    return (char*)lastChar;
}

/* 功能： 查找字符ch在字符串str中出现的次数
 */
uint32 strchrs(const char* str, const uint8 ch)
{
    ASSERT(str != NULL);
    uint32 chCnt = 0;
    const char* p = str;
    while (*p != 0)
    {
        if(*p == ch){
            chCnt++;
        }
        p++;
    }
    return chCnt;
}