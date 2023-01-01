#include "format.h"
#include "constant.h"
#include "string.h"
#include "number.h"
#include "assert.h"
enum Alignment{
    ALIGN_LFET      = 0,
    ALIGN_CENTER    = 1,
    ALIGN_RIGHT     = 2,
};
struct FormatData{
    char specifier;
    char padding;
    int width;
    enum Alignment align;
};
static int32 decodeFormat(const char* fmt, struct FormatData* formatData){
    bool setWidth = false;
    int32 count = 0;
    formatData->align = ALIGN_RIGHT;
    int32 width = 0;
    // 检查扩展
    char fmtChar = *(++fmt);
    if(fmtChar == '0'){
        formatData->padding = '0';
        fmtChar = *(++fmt);
        count += 1;
    }else{
        formatData->padding = ' '; 
    }
    // 检查宽度
    while('0' <= fmtChar && fmtChar <= '9'){
        setWidth = true;
        width = width * 10 + (fmtChar - '0');
        fmtChar = *(++fmt);
        count += 1;
    }
    if(setWidth){
        formatData->width = width;
    }else{
        formatData->width = -1;
    }
    // 检查数据格式
    // 此时fmtchar的内容应该为b, d, x, c, s
    formatData->specifier = fmtChar;
    count += 1;
    return count;  
}

uint32 format(char* str, const char* fmt, va_list ap){
    char numBuff[256] = {0};
    int strLen = 0;
    char* strBuff;
    int fillCount = 0;
    struct FormatData formatData;
    char* bufPtr = str;
    const char* indexPtr = fmt;
    char indexChar = *indexPtr;
    int32 argInt;
    char* argStr = "";
    while (indexChar)
    {
        if(indexChar != '%'){
            *(bufPtr++) = indexChar;
            indexChar = *(++indexPtr);
            continue;
        }
        // 获取字符格式化数据
        indexPtr += decodeFormat(indexPtr, &formatData);
        switch (formatData.specifier)
        {
        case 'b':
            argInt = va_arg(ap, int);
            memset(numBuff, '\0', 256);
            strBuff = numBuff;
            itoa(argInt, &strBuff, 2);
            strLen = strlen(numBuff);
            if(formatData.width == 0) break;
            if(formatData.width > 0){
                fillCount = formatData.width - strLen;
                if(fillCount > 0){
                    strrepeatapp(bufPtr, formatData.padding, fillCount);
                    bufPtr += fillCount;
                }
            }
            strcat(bufPtr, numBuff);
            bufPtr += strLen;
            break;
        case 'd':
            argInt = va_arg(ap, int);
            if(argInt < 0){
                argInt = 0 -argInt;
                *bufPtr++ = '-';
            }
            memset(numBuff, '\0', 256);
            strBuff = numBuff;
            itoa(argInt, &strBuff, 10);
            strLen = strlen(numBuff);
            if(formatData.width == 0) break;
            if(formatData.width > 0){
                fillCount = formatData.width - strLen;
                if(fillCount > 0){
                    strrepeatapp(bufPtr, formatData.padding, fillCount);
                    bufPtr += fillCount;
                }
            }
            strcat(bufPtr, numBuff);
            bufPtr += strLen;
            break;
        case 'x':
            argInt = va_arg(ap, int);
            memset(numBuff, '\0', 256);
            strBuff = numBuff;
            itoa(argInt, &strBuff, 16);
            strLen = strlen(numBuff);
            if(formatData.width == 0) break;
            if(formatData.width > 0){
                fillCount = formatData.width - strLen;
                if(fillCount > 0){
                    strrepeatapp(bufPtr, formatData.padding, fillCount);
                    bufPtr += fillCount;
                }
            }
            strcat(bufPtr, numBuff);
            bufPtr += strLen;
            break;
        case 'c':
            if(formatData.width == 0) break;
            if(formatData.width > 0){
                fillCount = formatData.width - strLen;
                if(fillCount > 0){
                    strrepeatapp(bufPtr, formatData.padding, fillCount);
                    bufPtr += fillCount;
                }
            }
            *(bufPtr++) = va_arg(ap, char);
            break;
        case 's':
            argStr = va_arg(ap, char*);
            if(formatData.width == 0) break;
            if(formatData.width > 0){
                fillCount = formatData.width - strLen;
                if(fillCount > 0){
                    strrepeatapp(bufPtr, formatData.padding, fillCount);
                    bufPtr += fillCount;
                }
            }
            strcat(bufPtr, argStr);
            bufPtr += strlen(argStr);
            break;
        default:
            painc("string format:: unexpected specifier");
            break;
        }
        indexChar = *(++indexPtr);
    }
    return strlen(str);
}