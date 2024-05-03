#ifndef YUI_SYSCALL_H
#define YUI_SYSCALL_H

#include <yui/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_EXIT,
    SYS_NR_FORK,
    SYS_NR_WRITE,
    SYS_NR_OPEN,
    SYS_NR_CLOSE,
    SYS_NR_WAITPID,
    SYS_NR_CREAT,
    SYS_NR_LINK,
    SYS_NR_UNLINK,
    SYS_NR_TIME,
    SYS_NR_GETPID, // 获取进程id
    SYS_NR_MKDIR,
    SYS_NR_RMDIR,
    SYS_NR_BRK,
    SYS_NR_UMASK,
    SYS_NR_GETPPID, // 获取父进程id
    SYS_NR_SLEEP,
    SYS_NR_YEILD,
} syscall_t;

u32 test();

void yeild();

void sleep(u32 ms);

int32 write(fd_t fd, char *buf, u32 len);

int32 brk(void *vaddr);

pid_t getpid();

pid_t getppid();

pid_t fork();

void exit(int status);

pid_t waitpid(pid_t pid, int32 *status);

time_t time();

mode_t umask(mode_t mask);

// 创建目录
int mkdir(char *pathname, int mode);

// 删除目录
int rmdir(char *pathname);

// 创建硬链接
int link(char *oldname, char *newname);

// 删除硬链接/删除文件
int unlink(char *filename);

// 打开文件
fd_t open(char *filename, int flags, int mode);

// 创建普通文件
fd_t creat(char *filename, int mode);

// 关闭文件
void close(fd_t fd);

#endif