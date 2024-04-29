#ifndef YUI_SYSCALL_H
#define YUI_SYSCALL_H

#include <yui/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_EXIT,
    SYS_NR_FORK,
    SYS_NR_WRITE,
    SYS_NR_GETPID, // 获取进程id
    SYS_NR_BRK,
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

#endif