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
#include <yui/global.h>
#include <yui/arena.h>
#include <yui/fs.h>

extern bitmap_t kernel_bitmap;
extern void task_switch(task_t *next);
extern tss_t tss;
extern file_t file_table[];

extern u32 volatile jiffies;
extern u32 jiffy;

#define NR_TASKS 64

// 存放进程指针的数组
static task_t *task_table[NR_TASKS];

// 默认阻塞链表，先进先出
static list_t block_list;
// 休眠链表，优先级队列，按睡眠结束的早晚排序
static list_t sleep_list;

static task_t *idle_task; // 0 号进程、空闲进程

// 从 task_table 里获得一个空闲任务
static task_t *get_free_task()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (task_table[i] == NULL)
        {
            // @todo free_kpage
            // 分配一页内存
            task_t *task = (task_t *)alloc_kpage(1);
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            task_table[i] = task;
            return task;
        }
    }
    panic("No more tasks, max is 64");
}

fd_t task_get_fd(task_t *task)
{
    fd_t i;
    for (i = 3; i < TASK_FILE_NR; i++)
    {
        if (!task->files[i])
            break;
    }
    if (i == TASK_FILE_NR)
    {
        panic("Exceed task max open files.");
    }
    return i;
}

void task_put_fd(task_t *task, fd_t fd)
{
    assert(fd < TASK_FILE_NR);
    task->files[fd] = NULL;
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

    // 没有就绪的任务则返回空闲进程
    if (task == NULL && state == TASK_READY)
    {
        task = idle_task;
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
    assert(task->node.next == NULL); // 该链表不在阻塞队列中
    assert(task->node.prev == NULL);

    if (blist == NULL)
    {
        blist = &block_list; // 未给出阻塞队列则使用默认阻塞队列
    }

    list_push(blist, &task->node);

    // 设置阻塞的状态不能为 就绪态 或 执行态
    assert(state != TASK_READY && state != TASK_RUNNING);

    task->state = state;

    task_t *current = running_task();
    if (current == task) // 如果当前进程为目标阻塞进程则进行进程调度
    {
        schedule();
    }
}

// 解除任务阻塞
void task_unblock(task_t *task)
{
    assert(!get_interrupt_state()); // 处于关中断

    list_remove(&task->node);

    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);

    task->state = TASK_READY;
}

// 激活任务
void task_activate(task_t *task)
{
    assert(task->magic == YUI_MAGIC);

    if (task->pde != get_cr3())
    {
        set_cr3(task->pde);
    }

    if (task->uid != KERNEL_USER)
    {
        tss.esp0 = (u32)task + PAGE_SIZE;
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

    // 只有当前进程为执行态时才能转为就绪态
    if (current->state == TASK_RUNNING)
    {
        current->state = TASK_READY;
    }

    if (!current->ticks)
    {
        current->ticks = current->priority;
    }

    next->state = TASK_RUNNING;

    if (next == current)
    {
        return;
    }

    task_activate(next);

    task_switch(next);
}

static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid)
{
    task_t *task = get_free_task();

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
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->gid = 0; // TODO
    task->vmap = &kernel_bitmap;
    task->pde = KERNEL_PAGE_DIR;
    task->brk = USER_EXEC_ADDR;
    task->text = USER_EXEC_ADDR;
    task->data = USER_EXEC_ADDR;
    task->end = USER_EXEC_ADDR;
    task->iexec = NULL;
    task->iroot = task->ipwd = get_root_inode();
    task->iroot->count += 2;

    task->pwd = (void *)alloc_kpage(1);
    strcpy(task->pwd, "/");

    task->umask = 0022; // 0755

    task->files[STDIN_FILENO] = &file_table[STDIN_FILENO];
    task->files[STDOUT_FILENO] = &file_table[STDOUT_FILENO];
    task->files[STDERR_FILENO] = &file_table[STDERR_FILENO];
    task->files[STDIN_FILENO]->count++;
    task->files[STDOUT_FILENO]->count++;
    task->files[STDERR_FILENO]->count++;

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

void task_sleep(u32 ms)
{
    assert(!get_interrupt_state());

    u32 ticks = ms / jiffy;        // 睡眠 ms 毫秒 所需的tick数
    ticks = ticks > 0 ? ticks : 1; // 至少休眠一个时间片

    task_t *current = running_task();
    current->ticks = jiffies + ticks; // 到期时间

    // 插入排序
    list_insert_sort(&sleep_list, &current->node,
                     element_node_offset(task_t, node, ticks));

    current->state = TASK_SLEEPING;

    schedule();
}

void task_wakeup()
{
    assert(!get_interrupt_state());

    list_t *list = &sleep_list;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;)
    {
        task_t *task = element_entry(task_t, node, ptr);

        if (task->ticks > jiffies)
        {
            break;
        }

        ptr = ptr->next;

        task->ticks = 0;

        task_unblock(task);
    }
}

extern void interrupt_exit();

static void task_build_stack(task_t *task)
{
    u32 addr = (u32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)addr;
    iframe->eax = 0;

    addr -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)addr;

    frame->ebp = 0xaa55aa55;
    frame->ebx = 0xaa55aa55;
    frame->edi = 0xaa55aa55;
    frame->esi = 0xaa55aa55;

    frame->eip = interrupt_exit;

    task->stack = (u32 *)frame;
}

pid_t task_fork()
{
    task_t *task = running_task();

    assert(task->node.next == NULL && // 进程不在就绪、阻塞队列，且处于执行态
           task->node.prev == NULL &&
           task->state == TASK_RUNNING);

    task_t *child = get_free_task(); // 创建新进程
    pid_t pid = child->pid;

    // 子进程的pid会被父进程覆盖
    memcpy(child, task, PAGE_SIZE);

    // pid
    child->pid = pid;
    child->ppid = task->pid;

    // 置为就绪态
    child->ticks = child->priority;
    child->state = TASK_READY;

    // 进程的位图
    child->vmap = kmalloc(sizeof(bitmap_t));
    memcpy(child->vmap, task->vmap, sizeof(bitmap_t));

    // 虚拟内存和内存位图
    void *buf = (void *)alloc_kpage(1);
    memcpy(buf, task->vmap->bits, PAGE_SIZE);
    child->vmap->bits = buf;

    // 页目录
    child->pde = (u32)copy_pde();

    // 拷贝 pwd
    child->pwd = (char *)alloc_kpage(1);
    strncpy(child->pwd, task->pwd, PAGE_SIZE);

    // 工作目录引用加一
    task->ipwd->count++;
    task->iroot->count++;

    if (task->iexec)
        task->iexec->count++;

    // 文件引用加一
    for (size_t i = 0; i < TASK_FILE_NR; i++)
    {
        file_t *file = child->files[i];
        if (file)
            file->count++;
    }

    task_build_stack(child);

    return child->pid;
}

// 等待标识符为 pid 子进程的结束，或发出信号
// pid 为 -1 则等待任意子进程
// 若找到一个结束的子进程则直接返回
// 若子进程未结束，则当前进程进入阻塞状态
pid_t task_waitpid(pid_t pid, int32 *status)
{
    task_t *task = running_task();
    task_t *child = NULL;

    while (true)
    {
        bool has_child = false;
        for (size_t i = 2; i < NR_TASKS; i++)
        {
            task_t *ptr = task_table[i];
            if (!ptr)
                continue;

            if (ptr->ppid != task->pid)
                continue;

            // TODO: pid = -1 表示任何子线程
            if (pid != ptr->pid && pid != -1)
                continue;

            // 子进程已结束，直接释放
            if (ptr->state == TASK_DIED)
            {
                child = ptr;
                task_table[i] = NULL;
                goto rollback;
            }

            has_child = true;
        }

        // 存在子进程且子进程未结束，当前进程阻塞，等待子进程结束
        if (has_child)
        {
            task->waitpid = pid;
            task_block(task, NULL, TASK_WAITING);
            continue;
        }
        break;
    }

rollback:
    *status = child->status;
    u32 ret = child->pid;
    // 释放PCB
    free_kpage((u32)child, 1);
    return ret;
}

extern int sys_execve();

void task_to_user_mode()
{
    task_t *task = running_task();

    task->vmap = kmalloc(sizeof(bitmap_t));

    void *buf = (void *)alloc_kpage(1);

    bitmap_init(task->vmap, buf, USER_MMAP_SIZE / PAGE_SIZE / 8, USER_MMAP_ADDR / PAGE_SIZE);

    // 创建用户进程页表
    task->pde = (u32)copy_pde();
    set_cr3(task->pde);

    u32 addr = (u32)task + PAGE_SIZE;

    addr -= sizeof(intr_frame_t);
    // 模拟中断 进入 用户态
    // 中断帧
    intr_frame_t *iframe = (intr_frame_t *)(addr);

    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;
    // ---以上用不到---

    // ---以下中断返回---
    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = YUI_MAGIC;

    iframe->eip = 0;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = USER_STACK_TOP; // 返回地址

    int err = sys_execve("/bin/init.out", NULL, NULL);
    panic("exec /bin/init.out failure");
}

void task_exit(int status)
{
    task_t *task = running_task();

    assert(task->node.prev == NULL &&
           task->node.next == NULL &&
           task->state == TASK_RUNNING);

    task->state = TASK_DIED;
    task->status = status;

    free_pde();

    free_kpage((u32)task->vmap->bits, 1);
    kfree(task->vmap);

    free_kpage((u32)task->pwd, 1);
    iput(task->ipwd);
    iput(task->iroot);
    iput(task->iexec);

    for (size_t i = 0; i < TASK_FILE_NR; i++)
    {
        file_t *file = task->files[i];
        if (file)
        {
            close(i);
        }
    }

    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *child = task_table[i];

        if (!child)
            continue;

        if (child->ppid != task->pid)
            continue;

        // 将子进程交给上级进程管理
        child->ppid = task->ppid;
    }
    DEBUG("task(pid=%d) exit with code %d...\n", task->pid, task->status);

    task_t *parent = task_table[task->ppid];
    // 父进程状态为等待且父进程等待
    if (parent->state == TASK_WAITING &&
        (parent->waitpid == -1 ||
         parent->waitpid == task->pid))
    {
        task_unblock(parent);
    }

    schedule();
}

extern void idle_thread();
extern void init_thread();
extern void test_thread();

void task_init()
{
    list_init(&block_list);
    list_init(&sleep_list);

    task_setup();

    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
    task_create(init_thread, "init", 5, NORMAL_USER);
    task_create(test_thread, "test", 5, NORMAL_USER);
}

pid_t sys_getpid()
{
    task_t *task = running_task();
    return task->pid;
}

pid_t sys_getppid()
{
    task_t *task = running_task();
    return task->ppid;
}