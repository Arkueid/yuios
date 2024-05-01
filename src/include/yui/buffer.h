#ifndef YUI_BUFFER_H
#define YUI_BUFFER_H

#include <yui/types.h>
#include <yui/list.h>
#include <yui/mutex.h>

#define BLOCK_SIZE 1024                       // 块大小
#define SECTOR_SIZE 512                       // 扇区大小
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE) // 一个块所占扇区数

typedef struct buffer_t
{
    char *data;        // 数据区
    dev_t dev;         // 设备号
    index_t block;     // 块号
    int count;         // 引用计数
    list_node_t hnode; // 哈希链表节点
    list_node_t rnode; // 缓冲节点
    lock_t lock;       // 锁
    bool dirty;        // 是否与磁盘内容一致
    bool valid;        // 是否有效
} buffer_t;

buffer_t *getblk(dev_t dev, index_t block);
buffer_t *bread(dev_t dev, index_t block);
void bwrite(buffer_t *bf);
void brelse(buffer_t *bf);

#endif