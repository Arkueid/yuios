#include <yui/syscall.h>

static _inline u32 _syscall0(u32 nr)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret) // 输出约束: ret=eax
        : "a"(nr)); // 输入约束：eax=nr
    return ret;
}

static _inline u32 _syscall1(u32 nr, u32 arg)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg));
    return ret;
}

static _inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

// 系统调用-test
u32 test()
{
    return _syscall0(SYS_NR_TEST);
}

// 系统调用-yeild
void yeild()
{
    _syscall0(SYS_NR_YEILD);
}

// 系统调用-sleep
void sleep(u32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}

int32 write(fd_t fd, char *buf, u32 len)
{
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

// 系统调用-brk
int32 brk(void *vaddr)
{
    _syscall1(SYS_NR_BRK, (u32)vaddr);
}

pid_t getpid()
{
    return _syscall0(SYS_NR_GETPID);
}

pid_t getppid()
{
    return _syscall0(SYS_NR_GETPPID);
}