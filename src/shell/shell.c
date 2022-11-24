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
// argc和argv必须为全局变量，便于全局调用
char* argv[MAX_ARG_NUM];
int32 argc = -1;
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
// 命令解析函数
static int32 cmdParse(char* cmdStr, char** argv, char token){
    ASSERT(cmdStr != NULL);
    int32 argIndex = 0;
    while(argIndex < MAX_ARG_NUM){
        argv[argIndex] = NULL;
        argIndex++;
    }
    char* next = cmdStr;
    int32  argc = 0;
    // 处理整行
    while(*next){
        // 去除命令行之间的空格
        while(*next == token){
            next++;
        }
        // 处理最后一个参数后接空格的情况
        if(*next == 0){
            break;
        }
        argv[argc] = next;
        // 处理每个命令子及参数
        while(*next && *next != token){
            next++;
        } 
        // 如果未结束（遇到token字符），使该token替换成0
        if(*next){
            *next++ = 0;
        }
        if(argc > MAX_ARG_NUM){
            return -1;
        }
        argc++; 
    }
    return argc;
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
        argc = -1;
        argc = cmdParse(cmdLine, argv, ' ');
        if(argc == -1){
            printf("number of arguments exeed %d\n", MAX_ARG_NUM);
        }
        int32 argIndex = 0;
        while(argIndex < argc){
            printf("%s ", argv[argIndex]);
            argIndex++;
        }
        printf("\n");
    }
}