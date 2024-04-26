#ifndef YUI_SYSCALL_H
#define YUI_SYSCALL_H

#include <yui/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_WRITE,
    SYS_NR_SLEEP,
    SYS_NR_YEILD,
} syscall_t;

u32 test();
void yeild();
void sleep(u32 ms);

int32 write(fd_t fd, char *buf, u32 len);

#endif