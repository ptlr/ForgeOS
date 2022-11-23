#ifndef FS_FS_H
#define FS_FS_H
#include "stdint.h"
#define MAX_PATH_LEN            1024            // 路径长度
#define MAX_FILE_NUM_PER_PART   4096            // 每个分区支持的最大文件数
#define BITS_PER_SECTOR         4096            // 每个扇区的位数
#define SECTOR_SIZE             512             // 每个扇区的大小
#define BLOCK_SIZE              SECTOR_SIZE     // 每个块的大小

/* 文件类型 */
enum FileType{
    FT_UNKNOWN  = 0,        // 不支持的文件类型
    FT_FILE     = 1,        // 普通文件
    FT_DIR      = 2         // 目录
};
/* 文件打开选项 */
enum OpenFlags{
    O_RDONLY,           // 只读
    O_WRONLY,           // 只写
    O_RDWR,             // 读写
    O_CREAT = 4         // 创建
};
// 文件读写位置偏移量
enum Whence {
    SEEK_SET = 1,
    SEEK_CUR = 2,
    SEEK_END = 3
};
struct PathSearchRecord{
    char searchPath[MAX_PATH_LEN];  // 查找文件过程中的父路劲
    struct Dir* parentDir;          // 文件所属的直接父目录
    enum FileType fileType;         // 文件类型
};
// 记录文件属性
struct Status{
    uint32 inodeNum;    // 文件inode
    uint32 size;        // 文件大小
    uint32 mtime;       // 最后更新时间
    uint32 ctime;       // 创建时间
    enum FileType fileType;// 问价类型
};
// 导出currentPart
extern struct Partition* currentPart;
// 返回路劲深度
int32 pathDepthCnt(char* path);
/* 搜索文件pathName, 如果找到返回inode号，否则返回-1 */
int searchFile(const char* pathName, struct PathSearchRecord* searchRecord);
void initFileSystem(int (* step)(void));
// 打开或创建文件后，返回文件描述符，否则返回-1
int32 sysOpen(const char* pathName, uint8 flags);
// 将buffer中连续count个字节写如文件描述符fd中，成功返回写入的字节数，失败放回-1
int32 sysWrite(uint32 fd, const void* buffer, uint32 count);
// 从文件描述符fd中读取count个字节到buffer,成功返回读取的字节数，失败返回-1
int32 sysRead(uint32 fd, void* buffer, uint32 count);
// 重置用于文件读写操作的偏移指针，成功时返回偏移量，失败时返回-1
int32 sysLSeek(int32 fd, int offset, uint8 whence);
/* 关闭文件描述符fd指向的文件，成功返回0， 否则返回-1 */
int32 sysClose(int32 fd);
// 删除文件，成功返回0，失败返回-1
int32 sysUnlink(const char* pathname);
// 创建目录pathName, 成功返回0，失败返回-1
int32 sysMkdir(const char* pathName);
// 打开目录，成功后返回目录指针，失败后返回NULL
struct Dir* sysOpendir(const char* name);
// 读取一个目录项，成功后返回其目录项地址，失败返回NULL
struct DirEntry* sysReadDirEntry(struct Dir* dir);
// 把目录的dirPos置0，以此实现目录回绕
void sysRewinddir(struct Dir* dir);
// 关闭目录，成功返回0，失败返回-1
int32 sysCloseDir(struct Dir* dir);
// 删除空目录, 成功返回0，失败返回-1
int32 sysRmdir(const char* pathName);
// 把当前工作目录绝对路径写入buffer, size是buffer的大小，当buffer为NULL时，由操作系统分配存储工作路径的空间并返回地址
char* sysGetCwd(char* buffer, uint32 size);
// 更改当前工作路径为绝对路径path，成功返回0，失败返回-1
int32 sysChdir(const char* path);
// buf中填充文件结构信息，成功返回0，失败返回-1
int32 sysStat(const char* path, struct Status* buffer);
#endif