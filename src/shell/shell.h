#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H
#include "fs.h"
// 路径保存
extern char finalPath[MAX_PATH_LEN];
// 输出终端提示
void printPrompt(void);
// 简单的shell
void forgeShell(void);
#endif