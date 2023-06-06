#include "buildin-cmd.h"
#include "fs.h"
#include "dir.h"
#include "debug.h"
#include "string.h"
#include "syscall.h"
#include "stdio.h"
#include "shell.h"
static void washPath(char* oldAbsPath, char* newAbsPath){
    ASSERT(oldAbsPath[0] == '/');
    char name[MAX_FILE_NAME_LEN] = {0};
    char* subPath = oldAbsPath;
    subPath = pathParse(subPath, name);
    if(name[0] == 0){
        newAbsPath[0] = '/';
        newAbsPath[1] = 0;
        return;
    }
    newAbsPath[0] = 0;
    strcat(newAbsPath,"/");
    while(name[0]){
        // 如果是上一级目录
        if(!strcmp("..", name)){
            char* slashPtr = strrchr(newAbsPath, '/');
            if(slashPtr != newAbsPath){
                *slashPtr = 0;
            }else{
                *(slashPtr + 1) = 0;
            }
        }else if(strcmp(".",name)){
            // 路径不是“.”
            if(strcmp(newAbsPath,"/")){
                // 最后一个字符不是“/”时添加一个“/”
                strcat(newAbsPath, "/");
            }
            strcat(newAbsPath, name);
        }
        memset(name, 0, MAX_FILE_NAME_LEN);
        if(subPath){
            subPath = pathParse(subPath, name);
        }
    }
}
// 获取绝对路径
void absPath(char* path, char* absPath){
    char mAbsPath[MAX_PATH_LEN] = {0};
    if(path[0] != '/'){
        memset(mAbsPath, 0, MAX_PATH_LEN);
        if(getcwd(mAbsPath, MAX_PATH_LEN) != NULL){
            if(!((mAbsPath[0] != '/') && (mAbsPath[1] != 0))){
                strcat(mAbsPath,"/");
            }
        }
    }
    strcat(mAbsPath, path);
    washPath(mAbsPath, absPath);
}
// pwd内建函数
void buildinPwd(uint32 argc, char** argv){
    if(argc > 1){
        printf("pwd: no argument support!");
        return;
    }else{
        if(NULL != getcwd(finalPath, MAX_PATH_LEN)){
            printf("%s\n", finalPath);
        }else{
            printf("pwd: get current work dir failed!\n");
        }
    }
}
// cd内建函数
char* buildinCd(uint32 argc, char** argv){
    if(argc > 2){
        printf("cd: only support 1 argument!\n");
        return NULL;
    }
    if(1 == argc){
        finalPath[0] = '/';
        finalPath[1] = 0;
    }else{
        absPath(argv[1], finalPath);
        //printf("ABSP: %s\n",finalPath);
    }
    if(chdir(finalPath) == -1){
        printf("cd: no such direcotry %s\n", finalPath);
        return NULL;
    }
    return finalPath;
}
// ls命令
void buildinLs(uint32 argc, char** argv){
    char* pathName = NULL;
    struct Status fileStatus;
    memset(&fileStatus, 0, sizeof(struct Status));
    bool longInfo = false;
    uint32 argPathNum = 0;
    uint32 argIndex = 1;    // 跳过argv[0], ls命令
    while(argIndex < argc){
        if(argv[argIndex][0] == '-'){ // 选项以-开始
            if(!strcmp("-l", argv[argIndex])){
                longInfo = true;
            }else if(!strcmp("-h", argv[argIndex])){
                printf("useage:\n-l list all information about the file\n-h for help\nlist all files in the current directory if no option\n");
                return;
            }
        }else{
            // 路径参数
            if(0 == argPathNum){
                pathName = argv[argIndex];
                argPathNum++;
            }else{
                printf("ls: only support one path!\n");
                return;
            }
        }
        argIndex++;
    }
    if(NULL == pathName){
        // 没有输入路径参数，设置为当前路径
        if(NULL != getcwd(finalPath, MAX_PATH_LEN)){
            pathName = finalPath;
        }else{
            printf("ls: getcwd for default path failed!\n");
            return;
        }
    }else{
        absPath(pathName, finalPath);
        pathName = finalPath;
    }
    // 获取输出文件信息
    if(-1 == stat(pathName, &fileStatus)){
        printf("ls: can't access '%s', no such file or dir\n", pathName);
        return;
    }
    // 根据情况输出信息
    if(FT_DIR == fileStatus.fileType){
        struct Dir* dir = opendir(pathName);
        struct DirEntry* dirEntryPtr = NULL;
        char subPathName[MAX_PATH_LEN] = {0};
        uint32 pathNameLen = strlen(pathName);
        uint32 lastCharIndex = pathNameLen - 1;
        memcpy(subPathName, pathName, pathNameLen);
        if(subPathName[lastCharIndex] != '/'){
            subPathName[pathNameLen] = '/';
            pathNameLen++;
        }
        rewinddir(dir);
        if(longInfo){
            char fileType;
            printf("Total:%d\n", fileStatus.size);
            while ((dirEntryPtr = readdir(dir)))
            {
                fileType = 'd';
                if(FT_FILE == dirEntryPtr->fileType){
                    fileType = '-';
                }
                subPathName[pathNameLen] = 0;
                strcat(subPathName, dirEntryPtr->fileName);
                memset(&fileStatus, 0, sizeof(struct Status));
                if(-1 == stat(subPathName, &fileStatus)){
                    printf("ls: can't access '%s', no such file dir\n", dirEntryPtr->fileName);
                    return;
                }
                printf("%c %4d %4d %s\n", fileType, fileStatus.inodeNum, fileStatus.size, dirEntryPtr->fileName);
            }
        }else{
            while((dirEntryPtr = readdir(dir))){
                printf("%s ", dirEntryPtr->fileName);
            }
            printf("\n");
        }
        closedir(dir);
    }else{
        // 文件
        if(longInfo){
            printf("- %4d %4d %s\n", fileStatus.inodeNum, fileStatus.size, pathName);
        }else{
            printf("%s\n", pathName);
        }
    }
}
// ps命令
void buildinPs(uint32 argc, char** argv){
    if(1 != argc){
        printf("ps: no argument support!\n");
        return;
    }
    ps();
}
// clear命令
void buildinClear(uint32 argc, char** argv){
    if(1 != argc){
        printf("ps: no argument support!\n");
        return;
    }
    clear();
}
// mkdir命令
int32 buildinMkdir(uint32 argc, char** argv){
    int32 retVal = -1;
    if(argc != 2){
        printf("mkdir: only support 1 argument!\n");
        return retVal;
    }
    absPath(argv[1], finalPath);
    // 如果创建的不是根目录
    if(strcmp("/", finalPath)){
        if(0 == mkdir(finalPath)){
            retVal = 0;
        }else{
            printf("mkdir: create dir '%s' failed!\n", argv[1]);
        }
    }
    return retVal;
}
// rmdir命令
int buildinRmdir(uint32 argc, char** argv){
    int32 retVal = -1;
    if(argc != 2){
        printf("rmdir: only support 1 argument!\n");
        return retVal;
    }
    absPath(argv[1], finalPath);
    // 如果创建的不是根目录
    if(strcmp("/", finalPath)){
        if(0 == rmdir(finalPath)){
            retVal = 0;
        }else{
            printf("rmdir: delete dir '%s' failed!\n", argv[1]);
        }
    }
    return retVal;
    
}
// rm命令
int buildinRm(uint32 argc, char** argv){
    int32 retVal = -1;
    if(argc != 2){
        printf("rm: only support 1 argument!\n");
        return retVal;
    }
    absPath(argv[1], finalPath);
    // 如果创建的不是根目录
    if(strcmp("/", finalPath)){
        if(0 == unlink(finalPath)){
            retVal = 0;
        }else{
            printf("rm: delete file '%s' failed!\n", argv[1]);
        }
    }
    return retVal;
}
// 帮助命令
int buildinHelp(char** argv, int32 flag){
    int32 retVal = -1;
    if(!flag){
        printf("'%s' is not a acceptable command, use:\n", argv[0]);
    }
    printf("command:\n");
    printf("help   show available command and shortcut key\n");
    printf("ls     list current directory files, options:\n       -l show details\n");
    printf("clear  clean screen\n");
    printf("cd     change current directorr\n");
    printf("ps     show current threads\n");
    printf("mkdir  create directory\n");
    printf("rmdir  delete directory\n");
    printf("rm     delete file\n");
    printf("shortcut key:\n");
    printf("CTRL + L clean screen\n");
    printf("CTRL + U clean input\n");
    retVal = 0;
    return retVal;
}