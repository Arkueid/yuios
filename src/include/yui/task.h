#ifndef YUI_TASK_H
#define YUI_TASK_H

#include <yui/types.h>
#include <yui/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1000

#define TASK_NAME_LEN 16

#define TASK_FILE_NR 16

// 入口地址
typedef void target_t();

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
    u32 *stack;                         // 内核栈
    list_node_t node;                   // 任务阻塞节点
    task_state_t state;                 // 任务状态
    u32 priority;                       // 任务优先级
    int ticks;                          // 剩余时间片
    u32 jiffies;                        // 上次执行时全局时间片
    char name[TASK_NAME_LEN];           // 任务名
    u32 uid;                            // 用户 id
    u32 gid;                            // 用户组 id
    pid_t pid;                          // 任务 id
    pid_t ppid;                         // 父任务 id
    u32 pde;                            // 页目录物理地址
    struct bitmap_t *vmap;              // 进程虚拟内存位图
    u32 text;                           // 代码段地址
    u32 data;                           // 数据段地址
    u32 end;                            // 程序结束地址
    u32 brk;                            // 进程堆内存最高地址
    int status;                         // 进程特殊状态
    pid_t waitpid;                      // 进程等待的 pid
    char *pwd;                          // 进程当前目录
    struct inode_t *ipwd;               // 进程当前目录 inode program work directory
    struct inode_t *iroot;              // 进程根目录 inode
    struct inode_t *iexec;              // 程序文件 inode
    u16 umask;                          // 进程用户权限
    struct file_t *files[TASK_FILE_NR]; // 进程文件表
    u32 magic;                          // 内核魔数，用于
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

typedef struct intr_frame_t
{
    u32 vector;

    u32 edi;
    u32 esi;
    u32 ebp;

    u32 esp_dummy;

    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;

    u32 vector0;

    u32 error;

    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;
} intr_frame_t;

void task_init();

// 获取当前正在执行的进程
task_t *running_task();
// 任务调度
void schedule();

// 进程主动让出执行权
void task_yield();

// 阻塞进程
void task_block(task_t *task, list_t *blist, task_state_t state);

// 唤醒进程
void task_unblock(task_t *task);

// 休眠
void task_sleep(u32 ms);
// 唤醒进程
void task_wakeup();

pid_t sys_getpid();
pid_t sys_getppid();

// 创建进程
pid_t task_fork();
// 退出进程
void task_exit(int status);

pid_t task_waitpid(pid_t pid, int32 *status);

void task_to_user_mode();

fd_t task_get_fd(task_t *task);

void task_put_fd(task_t *task, fd_t fd);

#endif