#ifndef YUI_MUTEX_H
#define YUI_MUTEX_H

#include <yui/types.h>
#include <yui/list.h>

typedef struct mutex_t
{
    bool value;  // 信号量
    list_t waiters; // 等待队列
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex); 

#endif