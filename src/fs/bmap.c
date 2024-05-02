#include <yui/fs.h>
#include <yui/debug.h>
#include <yui/bitmap.h>
#include <yui/assert.h>
#include <yui/string.h>
#include <yui/buffer.h>

index_t balloc(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    index_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        buf = sb->zmaps[i];
        assert(buf);

        // 将缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS + sb->desc->firstdatazone - 1);

        // 从位图中扫描一位
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {
            // 如果扫描成功，则 标记缓冲区为脏，终止查找
            assert(bit < sb->desc->zones);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf); // TODO
    return bit;
}

void bfree(dev_t dev, index_t idx)
{
    super_block_t *sb = get_super(dev);
    assert(sb != NULL);
    assert(idx < sb->desc->zones);

    buffer_t *buf;
    bitmap_t map;
    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        // 跳过开始的块
        if (idx > BLOCK_BITS * (i + 1))
        {
            continue;
        }

        buf = sb->zmaps[i];
        assert(buf);

        // 将整个缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, BLOCK_BITS * i + sb->desc->firstdatazone - 1);

        // 将 idx 对应的位置置为 0
        assert(bitmap_test(&map, idx));
        bitmap_set(&map, idx, 0);

        // 标记缓冲区脏
        buf->dirty = true;
        break;
    }
    bwrite(buf); // TODO
}

// 分配一个 inode
index_t ialloc(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    index_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < IMAP_NR; i++)
    {
        buf = sb->imaps[i];
        assert(buf);

        bitmap_make(&map, buf->data, BLOCK_BITS, i * BLOCK_BITS);
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {
            assert(bit < sb->desc->inodes);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf); // TODO
    return bit;
}

// 释放一个 inode
void ifree(dev_t dev, index_t idx)
{
    super_block_t *sb = get_super(dev);
    assert(sb != NULL);
    assert(idx < sb->desc->inodes);

    buffer_t *buf;
    bitmap_t map;

    for (size_t i = 0; i < IMAP_NR; i ++)
    {
        if (idx > BLOCK_BITS * (i + 1))
        {
            continue;
        }

        buf = sb->imaps[i];
        assert(buf);

        bitmap_make(&map, buf->data, BLOCK_BITS, i * BLOCK_BITS);
        assert(bitmap_test(&map, idx));

        bitmap_set(&map, idx, 0);
        buf->dirty = true;
        break;
    }
    bwrite(buf); // TODO
}