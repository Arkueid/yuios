#include <yui/fs.h>
#include <yui/syscall.h>
#include <yui/assert.h>
#include <yui/debug.h>
#include <yui/buffer.h>
#include <yui/arena.h>
#include <yui/string.h>
#include <yui/stdlib.h>
#include <yui/stat.h>
#include <yui/stat.h>

#define INODE_NR 64

static inode_t inode_table[INODE_NR];

// 申请一个 inode
static inode_t *get_free_inode()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        if (inode->dev == EOF)
        {
            return inode;
        }
    }
    panic("no more inode!!!");
}

// 释放一个 inode
static void put_free_inode(inode_t *inode)
{
    assert(inode != inode_table);
    assert(inode->count == 0);
    inode->dev = EOF;
}

// 获取根 inode
inode_t *get_root_inode()
{
    return inode_table;
}

// 计算 inode nr 对应的块号
static inline index_t inode_block(super_block_t *sb, index_t nr)
{
    // inode 编号从 1 开始
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + (nr - 1) / BLOCK_INODES;
}

// 从已有 inode 中查找编号为 nr 的 inode
static inode_t *find_inode(dev_t dev, index_t nr)
{
    super_block_t *sb = get_super(dev);
    assert(sb);
    list_t *list = &sb->inode_list;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        inode_t *inode = element_entry(inode_t, node, node);

        if (inode->nr == nr)
        {
            return inode;
        }
    }
    return NULL;
}

// 获得设备 dev 的 nr inode
inode_t *iget(dev_t dev, index_t nr)
{
    inode_t *inode = find_inode(dev, nr);
    if (inode)
    {
        inode->count++;
        inode->atime = time();

        return inode;
    }

    super_block_t *sb = get_super(dev);
    assert(sb);

    assert(nr <= sb->desc->inodes);

    inode = get_free_inode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count = 1;

    // 加入超级块 inode 链表
    list_push(&sb->inode_list, &inode->node);

    index_t block = inode_block(sb, inode->nr);
    buffer_t *buf = bread(inode->dev, block);

    inode->buf = buf;

    // 将缓冲视为一个 inode 描述符数组，获得对应的指针
    inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = inode->desc->mtime;
    inode->atime = time();

    return inode;
}

// 释放 inode
void iput(inode_t *inode)
{
    if (!inode)
        return;

    if (inode->buf->dirty)
    {
        bwrite(inode->buf);
    }

    inode->count--;

    if (inode->count)
    {
        return;
    }

    // 释放 inode 对应的缓冲
    brelse(inode->buf);

    // 从超级块链表中移除
    list_remove(&inode->node);

    // 释放 inode 内存
    put_free_inode(inode);
}

int inode_read(inode_t *inode, char *buf, u32 len, off_t offset)
{
    assert(ISFILE(inode->desc->mode) || ISDIR(inode->desc->mode));

    // 如果偏移量超过文件大小，返回 EOF
    if (offset >= inode->desc->size)
    {
        return EOF;
    }

    // 开始读取的位置
    u32 begin = offset;

    // 剩余字节数
    u32 left = MIN(len, inode->desc->size - offset);
    while (left)
    {
        // 找到对应文件的偏移，文件所在块
        index_t nr = bmap(inode, offset / BLOCK_SIZE, false);

        assert(nr);

        // 读取文件缓冲
        buffer_t *bf = bread(inode->dev, nr);

        // 文件块中的偏移量
        u32 start = offset % BLOCK_SIZE;

        // 本次需要读取的字节数
        u32 chars = MIN(BLOCK_SIZE - start, left);

        offset += chars;
        left -= chars;

        // 文件块中的指针
        char *ptr = bf->data + start;

        // 拷贝内容
        memcpy(buf, ptr, chars);

        // 更新缓冲位置
        buf += chars;

        // 释放文件缓冲块
        brelse(bf);
    }

    // 更新访问时间
    inode->atime = time();

    // 返回读取数量
    return offset - begin;
}

// 从 inode 的 offset 处，将 buf 的 len 个字节写入磁盘
int inode_write(inode_t *inode, char *buf, u32 len, off_t offset)
{
    // 不允许目录写入目录文件
    assert(ISFILE(inode->desc->mode));

    // 开始位置
    u32 begin = offset;

    // 剩余数量
    u32 left = len;

    while (left)
    {
        // 找到文件块，不存在则创建
        index_t nr = bmap(inode, offset / BLOCK_SIZE, true);

        assert(nr);

        // 读入文件块
        buffer_t *bf = bread(inode->dev, nr);
        bf->dirty = true;

        // 块中的偏移量
        u32 start = offset % BLOCK_SIZE;
        // 文件中的指针
        char *ptr = bf->data + start;

        // 读取的数量
        u32 chars = MIN(BLOCK_SIZE - start, left);

        // 更新偏移量
        offset += chars;

        left -= chars;

        // 如果偏移量大于文件大小，则更新文件大小
        if (offset > inode->desc->size)
        {
            inode->desc->size = offset;
            inode->buf->dirty = true;
        }

        // 拷贝内容
        memcpy(ptr, buf, chars);

        // 更新缓冲偏移
        buf += chars;

        // 释放文件块
        brelse(bf);
    }

    // 修改更新时间
    inode->desc->mtime = inode->atime = time();

    bwrite(inode->buf);

    return offset - begin;
}

static void inode_bfree(inode_t *inode, u16 *array, int index, int level)
{
    if (!array[index])
        return;
    
    if (!level)
    {
        bfree(inode->dev, array[index]);
        return;
    }

    buffer_t *buf = bread(inode->dev, array[index]);
    for (size_t i = 0; i < BLOCK_INDEXES; i ++)
    {
        inode_bfree(inode, (u16 *)buf->data, i, level - 1);
    }
    brelse(buf);
    bfree(inode->dev, array[index]);
}

// 释放 inode 所有文件块
void inode_truncate(inode_t *inode)
{
    if (!ISFILE(inode->desc->mode) && !ISDIR(inode->desc->mode))
        return;
    
    // 释放直接块
    for (size_t i = 0; i < DIRECT_BLOCK; i ++)
    {
        inode_bfree(inode, inode->desc->zone, i, 0);
        inode->desc->zone[i] = 0;
    }

    // 释放一级间接块
    inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK, 1);

    inode->desc->zone[DIRECT_BLOCK] = 0;

    // 释放二级间接块
    inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK + 1, 2);
    inode->desc->zone[DIRECT_BLOCK + 1] = 0;

    inode->desc->size = 0;
    inode->buf->dirty = true;
    inode->desc->mtime = time();
    bwrite(inode->buf);
} 

void inode_init()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        inode->dev = EOF;
    }
}