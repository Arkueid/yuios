#include <yui/mutex.h>
#include <yui/task.h>
#include <yui/interrupt.h>
#include <yui/assert.h>
#include  <yui/yui.h>

// 初始化信号量
void mutex_init(mutex_t *mutex)
{
    mutex->value = false;
    list_init(&mutex->waiters);
}


// 
void mutex_lock(mutex_t *mutex)
{
    bool intr = interrupt_disable();

    task_t *current = running_task();
    while (mutex->value == true)
    {
        // 阻塞后 当前进程在此处停止
        task_block(current, &mutex->waiters, TASK_BLOCKED);
        // 再次唤醒时进程从此处进入
        // 由进程调度 启动 的进程，默认是关中断的
    }

    // 没有其他线程获得该锁
    assert(mutex->value == false);

    // 释放改锁
    mutex->value ++;
    assert(mutex->value == true);

    // 恢复中断状态
    set_interrupt_state(intr);
}

void mutex_unlock(mutex_t *mutex)
{
    bool intr = interrupt_disable();

    assert(mutex->value == true);

    mutex->value--;
    assert(mutex->value == false);

    // 如果
    if (!list_empty(&mutex->waiters))
    {
        // 取出最早被阻塞的进程
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);

        assert(task->magic == YUI_MAGIC);

        // 转就绪态
        task_unblock(task);

        // 交出执行权，保证task能够获得mutex，防止饥饿状态
        task_yield(); // TODO:是否必要？
    }

    set_interrupt_state(intr);
}