#ifndef YUI_TASK_H
#define YUI_TASK_H

#include <yui/types.h>
#include <yui/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

// 入口地址
typedef u32 target_t();

typedef enum task_state_t
{
    TASK_INIT,     // 初始化
    TASK_RUNNING,  // 执行
    TASK_READY,    // 就绪
    TASK_BLOCKED,  // 阻塞
    TASK_SLEEPING, // 睡眠
    TASK_WAITING,  // 等待
    TASK_DIED,     // 死亡
} task_state_t;

typedef struct task_t
{
    u32 *stack;               // 内存栈，栈顶
    list_node_t node;         // 任务阻塞节点
    task_state_t state;       // 程序状态
    u32 prioriy;              // 优先级
    u32 ticks;                // 剩余执行时间
    u32 jiffies;              // 上次执行时全局时间
    char name[TASK_NAME_LEN]; // 任务名称
    u32 uid;                  // 用户id
    u32 pde;                  // 页目录物理地址
    struct bitmap_t *vmap;    // 进程虚拟内存位图
    u32 magic;                // 内核魔术，用于检测内存溢出
} task_t;

// 任务栈帧
typedef struct task_frame_t
{
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void); // 函数指针，入口地址
} task_frame_t;

void task_init();

// 获取当前正在执行的进程
task_t *running_task();
// 任务调度
void schedule();

// 进程主动让出执行权
void task_yield();

// 阻塞进程
void task_block(task_t *task, list_t *blist, task_state_t *state);
// 唤醒进程
void task_wake(task_t *task);

#endif