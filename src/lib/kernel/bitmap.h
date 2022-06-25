#ifndef LIB_KERNEL_BITMAP_H
#define LIB_KERNEL_BITMAP_H

#include "stdint.h"
#include "constant.h"
#define BITMAP_MASK 1

// 位图的结构体
struct Bitmap
{
    uint32 length;
    // 此处对应的数据单位是byte
    uint8* bits;
};
// 初始化bitmap
void initBitmap(struct Bitmap* bitmap);
// 测试bitmap的某位是否为1，为1返回true,否则返回false
bool testBitmap(struct Bitmap* bitmap, uint32 bitIndex);
// 在bitmap中申请count个位，成功返回起始下标，否则返回-1
int scanBitmap(struct Bitmap* bitmap, uint32 count);
// 将bitmap的bitIndex位设置为value
void setBitmap(struct Bitmap* bitmap, uint32 bitIndex, int8 value);
#endif