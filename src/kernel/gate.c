#include <yui/interrupt.h>
#include <yui/assert.h>
#include <yui/debug.h>
#include <yui/syscall.h>
#include <yui/task.h>

#define SYSCALL_SIZE 64

handler_t syscall_table[SYSCALL_SIZE];

extern void task_yield();

// 检查系统调用号是否正确
void syscall_check(u32 nr)
{
    if (nr >= SYSCALL_SIZE)
    {
        panic("syscall nr error!!!");
    }
}

// 默认系统调用函数
static void sys_default()
{
    panic("syscall not implemented!!!");
}

task_t *task = NULL;


// 系统调用-test
static u32 sys_test(u32 ebx, u32 ecx, u32 edx, u32 nr)
{
    // DEBUG("syscall test: ebx=0x%p, ecx=0x%p, edx=0x%p, nr=0x%p\n",
    //       ebx, ecx, edx, nr);

    if (!task)
    {
        task = running_task();
        task_block(task, NULL, TASK_BLOCKED);
    }
    else
    {
        task_wake(task);
        task = NULL;
    }
    return 255;
}

void syscall_init()
{
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = sys_default;
    }

    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_YEILD] = task_yield;
}
