#include <yui/buffer.h>
#include <yui/memory.h>
#include <yui/debug.h>
#include <yui/assert.h>
#include <yui/device.h>
#include <yui/string.h>
#include <yui/task.h>

#define HASH_COUNT 31 // TODO ？应该是个素数

static buffer_t *buffer_start = (buffer_t *)KERNEL_BUFFER_MEM;
static u32 buffer_count = 0;

// 记录当前 buffer_t 结构体位置
static buffer_t *buffer_ptr = (buffer_t *)KERNEL_BUFFER_MEM;

// 记录当前数据缓冲区位置
static void *buffer_data = (void *)(KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t free_list;              // 缓冲链表，被释放的块
static list_t wait_list;              // 等待进程的链表
static list_t hash_table[HASH_COUNT]; // 缓冲哈希表

// 哈希函数
u32 hash(dev_t dev, index_t block)
{
    return (dev ^ block) % HASH_COUNT;
}

static buffer_t *get_from_hash_table(dev_t dev, index_t block)
{
    u32 idx = hash(dev, block);
    list_t *list = &hash_table[idx];
    buffer_t *bf = NULL;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        buffer_t *ptr = element_entry(buffer_t, hnode, node);
        if (ptr->dev == dev && ptr->block == block)
        {
            bf = ptr;
            break;
        }
    }

    if (!bf)
    {
        return NULL;
    }

    // TODO bf 在空闲队列中则移除
    if (list_search(&free_list, &bf->rnode))
    {
        list_remove(&bf->rnode);
    }

    return bf;
}

// 将 bf 放入哈希表
static void hash_locate(buffer_t *bf)
{
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(!list_search(list, &bf->hnode));
    list_push(list, &bf->hnode);
}

// 将 bf 从哈希表中移除
static void hash_remove(buffer_t *bf)
{
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(list_search(list, &bf->hnode));
    list_remove(&bf->hnode);
}

// 直接初始化过慢，按需取用
static buffer_t *get_new_buffer()
{
    buffer_t *bf = NULL;

    if ((u32)buffer_ptr + sizeof(buffer_t) < (u32)buffer_data)
    {
        bf = buffer_ptr;
        bf->data = buffer_data;
        bf->dev = EOF;
        bf->block = 0;
        bf->count = 0;
        bf->dirty = false;
        bf->valid = false;
        lock_init(&bf->lock);
        buffer_count++;
        buffer_ptr++;
        // | buffer_ptr --增长--> 空闲区域 <--增长-- buffer_data |
        buffer_data -= BLOCK_SIZE;
        DEBUG("buffer count %d\n", buffer_count);
    }

    return bf;
}

// 获得空闲的 buffer
static buffer_t *get_free_buffer()
{
    buffer_t *bf = NULL;
    while (true)
    {
        bf = get_new_buffer();
        if (bf)
        {
            return bf;
        }

        // 从空闲列表中取得
        if (!list_empty(&free_list))
        {
            // 取最远未访问的块
            bf = element_entry(buffer_t, rnode, list_popback(&free_list));
            hash_remove(bf);
            bf->valid = false;
            return bf;
        }
        // 等待某个缓冲释放
        task_block(running_task(), &wait_list, TASK_BLOCKED);
    }
}

// 获得设备 dev，第 block 对应的缓冲
buffer_t *getblk(dev_t dev, index_t block)
{
    buffer_t *bf = get_from_hash_table(dev, block);
    if (bf)
    {
        bf->count++;
        return bf;
    }

    bf = get_free_buffer();
    assert(bf->count == 0);
    assert(bf->dirty == 0);

    bf->count = 1;
    bf->dev = dev;
    bf->block = block;

    // bf 放入哈希表
    hash_locate(bf);
    return bf;
}

// 读取 dev 的 block
buffer_t *bread(dev_t dev, index_t block)
{
    buffer_t *bf = getblk(dev, block);
    assert(bf != NULL);
    if (bf->valid)
    {
        return bf;
    }

    lock_acquire(&bf->lock);

    if (!bf->valid)
    {
        device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_READ);
        bf->dirty = false;
        bf->valid = true;
    }
    lock_release(&bf->lock);
    return bf;
}

// 写缓冲
void bwrite(buffer_t *bf)
{
    assert(bf);
    if (!bf->dirty)
        return;

    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_WRITE);
    bf->dirty = false;
    bf->valid = true;
}

// 释放缓冲
void brelse(buffer_t *bf)
{
    if (!bf)
        return;

    if (bf->dirty)
    {
        bwrite(bf);
    }

    bf->count--;
    assert(bf->count >= 0);
    if (bf->count)
        return;

    assert(!bf->rnode.next);
    assert(!bf->rnode.prev);
    list_push(&free_list, &bf->rnode);

    if (!list_empty(&wait_list))
    {
        task_t *task = element_entry(task_t, node, list_popback(&wait_list));
        task_unblock(task);
    }
}

void buffer_init()
{
    DEBUG("buffer_t size is %d\n", sizeof(buffer_t));

    // 初始化空闲链表
    list_init(&free_list);
    // 初始化等待进程链表
    list_init(&wait_list);

    // 初始化哈希表
    for (size_t i = 0; i < HASH_COUNT; i++)
    {
        list_init(&hash_table[i]);
    }
}
