#ifndef SHELL_SHELL_CMD_H
#define SHELL_SHELL_CMD_H
#include "stdint.h"
#include "fs.h"
// 获取绝对路径
void absPath(char* path, char* absPath);
// pwd内建函数
void buildinPwd(uint32 argc, char** argv);
// cd内建函数
char* buildinCd(uint32 argc, char** argv);
// ls命令
void buildinLs(uint32 argc, char** argv);
// ps命令
void buildinPs(uint32 argc, char** argv);
// clear命令
void buildinClear(uint32 argc, char** argv);
// mkdir命令
int32 buildinMkdir(uint32 argc, char** argv);
// rmdir命令
int buildinRmdir(uint32 argc, char** argv);
// rm命令
int buildinRm(uint32 argc, char** argv);
// 帮助命令
int buildinHelp(char** argv, int32 flag);
#endif