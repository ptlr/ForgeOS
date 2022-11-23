#include "shell.h"
#include "stdio.h"
#include "debug.h"
#include "constant.h"
#include "syscall.h"
#include "fs.h"
#include "file.h"
#include "string.h"
#include "syscall.h"

#define CMD_LENGTH 128
#define MAX_ARG_NUM 16

// 存储输入的命令
static char cmdLine[CMD_LENGTH] = {0};
// 用来缓存当前的目录
char cwDCache[128] = {0};
// 输出终端提示
void printPrompt(void){
    printf("[ptlr@forge %s]\n$ ", cwDCache);
}
// 从键盘缓冲区读取最多count个字符到buffer中
static void readLine(char* buffer, int32 count){
    ASSERT(buffer != NULL && count > 0);
    char* pos = buffer;
    while (read(STD_IN, pos, 1) != -1 && (pos - buffer) < count)
    {
        switch (*pos)
        {
            // ctrl + l 清屏
            case 'l' - 'a':
                *pos = 0;
                clsScreen();
                printPrompt();
                printf("%s", buffer);
            break;
            // ctrl + u 清除输入
            case 'u' - 'a':
                while(buffer != pos){
                    putchar('\b');
                    *(pos--) = 0;
                }
            break;
            // 处理回车
            case '\n':
            case '\r':
                *pos = 0;
                putchar('\n');
                return;
            break;
            case '\b':
                if(buffer[0] != '\b'){
                    --pos;
                    putchar('\b');
                }
            break;
            default:
                putchar(*pos);
                pos++;
            break;
        }
    }
    printf("readLine: can't find enter key in the cmdLine, max number of char is 128\n");
}
// 简单的shell
void forgeShell(void){
    cwDCache[0] = '/';
    while (1)
    {
        printPrompt();
        memset(cmdLine, 0, CMD_LENGTH);
        readLine(cmdLine, CMD_LENGTH);
        // 单纯输入回车
        if(cmdLine[0] == 0){
            continue;
        }
    }
}