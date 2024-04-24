#include <yui/interrupt.h>
#include <yui/syscall.h>
#include <yui/debug.h>

void idle_thread()
{
    set_interrupt_state(true);

    u32 counter = 0;
    while (true)
    {
        DEBUG("idle task...%d\n", counter++);
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭 CPU，进入暂停状态
        );
        yeild(); // 放弃执行权，调度执行其他程序
    }
}

void init_thread()
{
    set_interrupt_state(true);

    while (true)
    {
        DEBUG("init task...\n");
        // test();
    }
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        DEBUG("test task %d...\n", counter++);
        sleep(709);
    }
}