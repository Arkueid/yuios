#include <yui/task.h>
#include <yui/printk.h>
#include <yui/debug.h>

#define PAGE_SIZE 0x1000

task_t *a = (task_t *)0x1000;
task_t *b = (task_t *)0x2000;

extern void task_switch(task_t *next);

// 创建进程
static void task_create(task_t *task, target_t target)
{
    // 栈顶为高地址
    u32 stack = (u32)task + PAGE_SIZE;
    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    task->stack = (u32 *)stack;
}

// 获取当前进程
task_t *running_task()
{
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n");
    // eax存放返回地址
    // 相当于 return task_t
}

// 进程调度
void schedule()
{
    task_t *current = running_task();
    // 模拟进程调度
    task_t *next = current == a ? b : a;
    task_switch(next);
}

u32 thread_a()
{
    while (true)
    {
        printk("A");
        schedule();
    }
}

u32 thread_b()
{
    while (true)
    {
        printk("B");
        schedule();
    }
}

void task_init()
{
    task_create(a, thread_a);
    task_create(b, thread_b);
    schedule();
}