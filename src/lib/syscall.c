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

u32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void yeild()
{
    _syscall0(SYS_NR_YEILD);
}