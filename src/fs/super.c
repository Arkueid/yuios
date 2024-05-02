#include <yui/fs.h>
#include <yui/buffer.h>
#include <yui/device.h>
#include <yui/assert.h>
#include <yui/string.h>
#include <yui/debug.h>

#define SUPER_NR 16  // 超级块数量

static super_block_t super_table[SUPER_NR]; // 超级根块
static super_block_t *root;                 // 根文件系统超级块

static super_block_t *get_free_super()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == EOF)
        {
            return sb;
        }
    }
    panic("reached max num of super blocks!!!");
}

// 获取设备dev的超级块
super_block_t *get_super(dev_t dev)
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == dev)
        {
            return sb;
        }
    }
    return NULL;
}

super_block_t *read_super(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    if (sb)
    {
        return sb;
    }

    DEBUG("reading super block of device %d\n", dev);

    // 获得空闲超级块
    sb = get_free_super();
    buffer_t *buf = bread(dev, 1);
    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINIX1_MAGIC);

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    int idx = 2; // 块位图从第 2 块开始，第 0 块引导快，第 1 块超级块

    for (int i = 0; i < sb->desc->imap_blocks; i++)
    {
        assert(i < IMAP_NR);
        if ((sb->imaps[i] = bread(dev, idx)))
            idx++;
        else
            break;
    }

    for (int i = 0; i < sb->desc->zmap_blocks; i++)
    {
        assert(i < ZMAP_NR);
        if ((sb->zmaps[i] = bread(dev, idx)))
            idx++;
        else
            break;
    }

    return sb;
}

// 挂载根文件系统
static void mount_root()
{
    DEBUG("Mount root file system...\n");

    // 假设主硬盘第一个分区是根文件系统
    device_t *device = device_find(DEV_IDE_PART, 0);
    assert(device);

    root = read_super(device->dev);
}

void super_init()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        sb->dev = EOF;
        sb->desc = NULL;
        sb->buf = NULL;
        sb->iroot = NULL;
        sb->imount = NULL;
        list_init(&sb->inode_list);
    }

    mount_root();
}