#ifndef YUI_SYSCALL_H
#define YUI_SYSCALL_H

#include <yui/types.h>
#include <yui/stat.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_EXIT,
    SYS_NR_FORK,
    SYS_NR_READ,
    SYS_NR_WRITE,
    SYS_NR_OPEN,
    SYS_NR_CLOSE,
    SYS_NR_WAITPID,
    SYS_NR_CREAT,
    SYS_NR_LINK,
    SYS_NR_UNLINK,
    SYS_NR_EXECVE,
    SYS_NR_CHDIR,
    SYS_NR_TIME,
    SYS_NR_MKNOD,
    SYS_NR_STAT,
    SYS_NR_LSEEK,
    SYS_NR_GETPID, // 获取进程id
    SYS_NR_MOUNT,
    SYS_NR_UMOUNT,
    SYS_NR_FSTAT,
    SYS_NR_READDIR,
    SYS_NR_MMAP,
    SYS_NR_MUNMAP,
    SYS_NR_MKDIR,
    SYS_NR_RMDIR,
    SYS_NR_DUP,
    SYS_NR_PIPE,
    SYS_NR_BRK,
    SYS_NR_UMASK,
    SYS_NR_CHROOT,
    SYS_NR_DUP2,
    SYS_NR_GETPPID, // 获取父进程id
    SYS_NR_SLEEP,
    SYS_NR_YEILD,
    SYS_NR_GETCWD,
    SYS_NR_CLEAR,
    SYS_NR_MKFS,
} syscall_t;

enum mmap_type_t
{
    PROT_NONE = 0,
    PROT_READ = 1,
    PROT_WRITE = 2,
    PROT_EXEC = 4,

    MAP_SHARED = 1,
    MAP_PRIVATE = 2,
    MAP_FIXED = 0x10,
};

u32 test();

void yeild();

void sleep(u32 ms);

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

// 读文件
int read(fd_t fd, char *buf, int len);

// 写文件
int write(fd_t fd, char *buf, int len);

// 设置文件偏移量
int lseek(fd_t fd, off_t offset, int whence);

// 获取当前路径
char *getcwd(char *buf, size_t size);

// 切换当前目录
int chdir(char *pathname);

// 切换根目录
int chroot(char *pathname);

// 读取目录
int readdir(fd_t fd, void *dir, int count);

// 清屏
void clear();

// 获取文件状态
int stat(char *filename, stat_t *statbuf);
int fstat(fd_t fd, stat_t *statbuf);

// 创建设备文件
int mknod(char *filename, int mode, int dev);

// 挂载设备
int mount(char *devname, char *dirname, int flags);
// 卸载设备
int umount(char *target);

// 格式化文件系统
int mkfs(char *devname, int icount);

int brk(void *addr);

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void *addr, size_t length);

// 执行程序
int execve(char *filename, char *argv[], char *envp[]);

// 复制文件描述符
fd_t dup(fd_t oldfd);

fd_t dup2(fd_t oldfd, fd_t newfd);

// 创建管道
int pipe(fd_t pipefd[2]);

#endif