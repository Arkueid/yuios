#include <yui/task.h>
#include <yui/types.h>
#include <yui/printk.h>
#include <yui/debug.h>
#include <yui/bitmap.h>
#include <yui/memory.h>
#include <yui/assert.h>
#include <yui/interrupt.h>
#include <yui/string.h>
#include <yui/yui.h>
#include <yui/syscall.h>

extern bitmap_t kernel_bitmap;
extern void task_switch(task_t *next);

#define NR_TASKS 64

// 存放进程指针的数组
static task_t *task_table[NR_TASKS];
static list_t block_list;  // 默认阻塞链表

// 从 task_table 里获得一个空闲任务
static task_t *get_free_task()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (task_table[i] == NULL)
        {
            // @todo free_kpage
            // 分配一页内存
            task_table[i] = (task_t *)alloc_kpage(1);
            return task_table[i];
        }
    }
    panic("No more tasks, max is %d\n", NR_TASKS);
}

// 从任务队列中查找某种状态的任务，自身除外
static task_t *task_search(task_state_t state)
{
    // 关中断状态下执行
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *current = running_task();

    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *ptr = task_table[i];
        if (ptr == NULL)
            continue;

        if (ptr->state != state)
            continue;
        if (current == ptr)
            continue;
        if (task == NULL ||              // 任务的状态满足要求
            task->ticks < ptr->ticks ||  // 此处，就绪队列中表示优先级；执行态的进程则比较剩余执行时间最大的
            ptr->jiffies < task->jiffies // 选取最晚进入队列的
        )
            task = ptr;
    }

    return task;
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

/// @brief 阻塞进程
/// @param task 进程控制块指针
/// @param blist 阻塞队列，为Null则使用默认阻塞队列
/// @param state 进程状态
void task_block(task_t *task, list_t *blist, task_state_t state)
{
    assert(!get_interrupt_state());  // 进程阻塞需要关中断
    assert(task->node.next == NULL);  // 该链表不在阻塞队列中
    assert(task->node.prev == NULL);

    if (blist == NULL)
    {
        blist = &block_list;  // 未给出阻塞队列则使用默认阻塞队列
    }

    list_push(blist, &task->node);

    // 设置阻塞的状态不能为 就绪态 或 执行态
    assert(state != TASK_READY && state != TASK_RUNNING);

    task->state = task->state;

    task_t *current = running_task();
    if (current == task) // 如果当前进程为目标阻塞进程则进行进程调度
    {
        schedule();
    }
}

// 进程调度
void schedule()
{
    // 处于关中断状态
    assert(!get_interrupt_state());

    task_t *current = running_task();
    // 从就绪队列中查找一个合适的进程
    task_t *next = task_search(TASK_READY);

    assert(next != NULL);
    assert(next->magic == YUI_MAGIC);

    current->state = TASK_READY;

    next->state = TASK_RUNNING;

    if (next == current)
    {
        return;
    }

    task_switch(next);
}

static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid)
{
    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);

    // 从高地址向低地址增长
    // 进程的内存空间中 最低地址开始 存放进程控制信息
    // 进程栈栈底在进程内存空间的最高处 开始存放
    u32 stack = (u32)task + PAGE_SIZE;

    // 留出 存放 硬件上下文信息 的空间
    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;

    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    strcpy((char *)task->name, name);

    task->stack = (u32 *)stack;
    task->prioriy = priority;
    task->ticks = task->prioriy;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_bitmap; // TODO: 为什么需要使用vmap
    task->pde = KERNEL_PAGE_DIR;
    // 如果栈顶移动到魔数的位置，或者该处魔数被修改
    // 说明栈溢出
    task->magic = YUI_MAGIC;

    return task;
}

static void task_setup()
{
    task_t *task = running_task();
    task->magic = YUI_MAGIC;
    task->ticks = 1;

    memset(task_table, 0, sizeof(task_table));
}

void task_yield()
{
    schedule();
}

u32 thread_a()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("A");
        test();
    }
}

u32 thread_b()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("B");
        test();
    }
}

u32 thread_c()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("C");
        test();
    }
}

void task_init()
{
    list_init(&block_list);
    
    task_setup();

    task_create(thread_a, "a", 5, KERNEL_USER);
    task_create(thread_b, "b", 5, KERNEL_USER);
    task_create(thread_c, "c", 5, KERNEL_USER);
}