#include <yui/interrupt.h>
#include <yui/syscall.h>
#include <yui/debug.h>
#include <yui/mutex.h>
#include <yui/printk.h>
#include <yui/task.h>
#include <yui/stdio.h>
#include <yui/arena.h>
#include <yui/fs.h>
#include <yui/string.h>

void idle_thread()
{
    set_interrupt_state(true);
    while (true)
    {
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭 CPU，进入暂停状态
        );
        yeild(); // 放弃执行权，调度执行其他程序
    }
}

extern void dev_init();

void init_thread()
{
    char tmp[100]; // 在任务栈栈底创建一个 中断帧
    dev_init();
    task_to_user_mode();
}

void test_thread()
{
    set_interrupt_state(true);

    while (true)
    {
        sleep(1000);
    }
}
