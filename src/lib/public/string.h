#include "constant.h"
#include "stdint.h"

#ifndef LIB_STRING_H
#define LIB_STRING_H

//内存相关

/* 功能：将从dst_开始size个字节设置为value
 */
void memset(void* dst_, uint8 value, uint32 size);
/* 功能： 将src_开始的size个字节复制到dst_
 */
void memcpy(void* dst_, void* src_, uint32 size);
/* 功能： 连续比较a_和b_开头的开头的size个字节
 * 返回： 如果都相同返回0,若a_ > b_返回1，否则返回-1
 */
int memcmp(void* a_m, void* b_, uint32 size);

// 字符串相关
/* 功能：将字符串从src_复制到dst_
 */
char* strcpy(char* dst_, const char* src_);

/* 功能： 将src_拼接到dst_后, 返回拼接后的地址
 */
char* strcat(char* dst_, const char* src_);

/* 功能： 返回字符串的长度
 */
uint32 strlen(const char* str);

/* 功能：比较两个字符串
 * 返回：a > b时返回1, 相等时返回0, 否则返回-1
 */
int8 strcmp(const char* a, const char* b);

/* 功能： 从左到右查找字符串str中首次出现ch的地址
 */
char* strchr(const char* str, const uint8 ch);

/* 功能： 从后往前查找字符串str中首次出现ch的地址
 */
char* strrchr(const char* str, const uint8 ch);

/* 功能： 查找字符ch在字符串str中出现的次数
 *
 */
uint32 strchrs(const char* str, const uint8 ch);

/*
 * 格式化字符串
 * 
 */
void format(char* buffer, const char* format, ...);
#endif