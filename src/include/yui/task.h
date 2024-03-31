#ifndef YUI_TASK_H
#define YUI_TASK_H

#include <yui/types.h>

// 入口地址
typedef u32 target_t();

typedef struct task_t
{
    u32 *stack; // 内存栈，栈顶
} task_t;

typedef struct task_frame_t
{
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);  // 函数指针，入口地址
} task_frame_t;

void task_init();

#endif