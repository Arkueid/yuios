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
extern void hang();

static void user_init_thread()
{

    char buf[256];
    chroot("/d1");
    chdir("/d2");
    getcwd(buf, sizeof(buf));
    printf("current work directory: %s\n", buf);

    while (true)
    {
        // printf("user thread %d\n", time());
        char ch;
        read(stdin, &ch, 1);
        write(stdout, &ch, 1);
    }
}

void init_thread()
{
    intr_frame_t iframe; // 在任务栈栈底创建一个 中断帧
    task_to_user_mode(user_init_thread);
}

void test_thread()
{
    set_interrupt_state(true);

    DEBUG("test finished of task %d\n", getpid());

    while (true)
    {
        test();
        sleep(10);
    }
}
