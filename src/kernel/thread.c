#include <yui/interrupt.h>
#include <yui/syscall.h>
#include <yui/debug.h>
#include <yui/mutex.h>
#include <yui/printk.h>
#include <yui/task.h>
#include <yui/stdio.h>
#include <yui/arena.h>

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
    u32 counter = 0;

    char ch;
    while (true)
    {
        pid_t pid = fork();

        if (pid)
        {
            printf("fork: parent(pid=%d, ppid=%d): return pid=%d\n", getpid(), getppid(), pid);
        }
        else
        {
            printf("fork: child(pid=%d, ppid=%d): return pid=%d\n", getpid(), getppid(), pid);
            exit(0);
        }
        sleep(1000);
    }
}

void init_thread()
{
    intr_frame_t iframe;  // 在任务栈栈底创建一个 中断帧
    task_to_user_mode(user_init_thread);
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        // printf("test thread: pid=%d, ppid=%d...\n", getpid(), getppid());
        sleep(2000);
    }
}

