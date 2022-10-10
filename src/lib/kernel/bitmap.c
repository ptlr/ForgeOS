#include "bitmap.h"
#include "string.h"
#include "print.h"
#include "debug.h"
#include "stdio.h"
#include "console.h"
// 初始化bitmap
void initBitmap(struct Bitmap* bitmap)
{
    //printf("PADDR = 0x%x, LEN = 0x%x\n", bitmap->bits, bitmap->length);
    //while(1);
    memset(bitmap->bits, 0, bitmap->length);
}
// 测试bitmap的某位是否为1，为1返回true,否则返回false
bool testBitmap(struct Bitmap* bitmap, uint32 bitIndex)
{
    // 向下取整，获得对应Byte的index
    uint32 byteIndex = bitIndex / 8;
    // 获取Byte内部对应的bit位置
    uint32 bitOdd = bitIndex % 8;
    return (bitmap->bits[byteIndex] & (BITMAP_MASK << bitOdd));
}
// 在bitmap中申请count个位，成功返回起始下标，否则返回-1
int scanBitmap(struct Bitmap* bitmap, uint32 count)
{
    uint32 byteIndex = 0;
    // 找到有空闲的第一个字节
    while((0xFF == bitmap->bits[byteIndex]) && (byteIndex < bitmap->length))
    {
        byteIndex++;
    }
    ASSERT(byteIndex < bitmap->length);
    // 找不到空闲的位置，返回-1
    if(byteIndex == bitmap->length)
    {
        return -1;
    }
    // 在byteIndex字节对应的位置找到第一个闲置的bit位置
    int bitIndex = 0;
    while(((uint8)BITMAP_MASK << bitIndex) & bitmap->bits[byteIndex])
    {
        bitIndex++;
    }
    // 计算出开始空闲位
    int bitIndexStart = byteIndex * 8 + bitIndex;
    if(1 == count)
    {
        return bitIndexStart;
    }
    uint32 bitLeft = bitmap->length - bitIndexStart; // 计算出还有多少剩余bit个数
    uint32 nextBit = bitIndexStart + 1;
    uint32 bitCount = 1;    // 包括目前的一位
    bitIndexStart = -1;     // 如果找不到直接返回
    while(bitLeft-- > 0)
    {
        if(0 == testBitmap(bitmap, nextBit))
        {
            bitCount++;
        }else{
            bitCount = 0;
        }
        if(bitCount == count){
            // 计算出开始位置
            bitIndexStart = nextBit - count + 1;
            break;
        }
        nextBit++;
    }
    return bitIndexStart;
    
}
// 将bitmap的bitIndex位设置为value
void setBitmap(struct Bitmap* bitmap, uint32 bitIndex, int8 value)
{
    ASSERT(value == 0 || value == 1);
    uint32 byteIndex = bitIndex / 8; //向下取整用于获取比特对应的index
    uint8 bitOdd = bitIndex % 8; // 取余，用于标识比特内的index
    uint8 opVal = (BITMAP_MASK << bitOdd);
    if(value == 1){
        bitmap->bits[byteIndex] |= opVal;
    }else if(value == 0){
        bitmap->bits[byteIndex] &= ~opVal;
    }
}