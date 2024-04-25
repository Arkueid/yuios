#include <yui/interrupt.h>
#include <yui/syscall.h>
#include <yui/debug.h>
#include <yui/mutex.h>
#include <yui/printk.h>

void idle_thread()
{
    set_interrupt_state(true);

    u32 counter = 0;
    while (true)
    {
        // DEBUG("idle task...%d\n", counter++);
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭 CPU，进入暂停状态
        );
        yeild(); // 放弃执行权，调度执行其他程序
    }
}

lock_t mutex;

extern void keyboard_read(char *ch, u32 count);

void init_thread()
{

    set_interrupt_state(true);
    u32 counter = 0;

    char ch;
    while (true)
    {
        bool intr = interrupt_disable();
        
        keyboard_read(&ch, 1);
        printk("%c", ch);
        set_interrupt_state(intr);
    }
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        // DEBUG("test task %d...\n", counter++);
        sleep(700);
    }
}