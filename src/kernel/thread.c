#include <yui/interrupt.h>
#include <yui/syscall.h>
#include <yui/debug.h>
#include <yui/mutex.h>

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

lock_t mutex;

void init_thread()
{
    lock_init(&mutex);
    set_interrupt_state(true);

    while (true)
    {
        lock_accquire(&mutex);
        DEBUG("init task...\n");
        // test();
        lock_release(&mutex);
    }
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        lock_accquire(&mutex);
        DEBUG("test task %d...\n", counter++);
        lock_release(&mutex);
    }
}