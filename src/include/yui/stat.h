#ifndef YUI_STAT_H
#define YUI_STAT_H

#include <yui/types.h>

// 文件类型
#define IFMT 00170000 // 文件类型（8进制表示）
#define IFREG 0100000 // 常规文件
#define IFBLK 0060000 // TODO 块特殊设备文件
#define IFDIR 0040000 // 目录文件
#define IFCHR 0020000 // 字符设备文件
#define IFIFO 0010000 // FIFO 特殊文件
#define IFSYM 0120000 // 符号链接

// 文件属性
#define ISUID 0004000 // 执行时设置用户 id
#define ISGID 0002000 // 执行时设置组 id

#define ISVTX 0001000 // 对于目录，受限删除标志

#define ISREG(m) (((m) & IFMT) == IFREG)  // 是常规文件
#define ISDIR(m) (((m) & IFMT) == IFDIR)  // 是目录文件
#define ISCHR(m) (((m) & IFMT) == IFCHR)  // 是字符设备文件
#define ISBLK(m) (((m) & IFMT) == IFBLK)  // 是块设备文件
#define ISFIFO(m) (((m) & IFMT) == IFIFO) // 是 FIFO 特殊文件

#define ISSYM(m) (((m) & IFMT) == IFSYM) // 是符号链接

// 宿主权限
#define IRWXU 00700 // 宿主可以读、写、执行/搜索
#define IRUSR 00400 // 宿主可以读
#define IWUSR 00200 // 宿主可以写
#define IXUSR 00100 // 宿主可以执行/搜索

// 组成员权限
#define IRWXG 00070 // 读写执行搜索
#define IRGRP 00040 // 读
#define IWGRP 00020 // 写
#define IXGRP 00010 // 执行、搜索

// 其他人权限
#define IRWXO 00007 // 读写执行搜索
#define IROTH 00004 // 读
#define IWOTH 00002 // 写
#define IXOTH 00001 // 执行

typedef struct stat_t
{
    dev_t dev;    // 存储文件的设备号
    index_t nr;   // 文件 inode 的节点号
    u16 mode;     // 文件类型和属性
    u8 nlinks;    // 文件的链接数
    u16 uid;      // 文件的用户号
    u8 gid;       // 文件的组号
    dev_t rdev;   // 设备号
    size_t size;  // 文件大小
    time_t atime; // 上次访问时间
    time_t mtime; // 最后修改时间
    time_t ctime; // 创建时间
} stat_t;

#endif