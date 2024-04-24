#include <yui/syscall.h>

static _inline u32 _syscall0(u32 nr)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "a="(ret) // 输出约束: ret=eax
        : "a"(nr)); // 输入约束：eax=nr
    return ret;
}

static _inline u32 _syscall1(u32 nr, u32 arg)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "a="(ret)
        : "a"(nr), "b"(arg)
    );
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