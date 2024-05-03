#include <yui/fs.h>
#include <yui/buffer.h>
#include <yui/stat.h>
#include <yui/syscall.h>
#include <yui/string.h>
#include <yui/assert.h>
#include <yui/debug.h>

// 判断文件名是否相等
static bool match_name(const char *name, const char *entry_name, char **next)
{
    char *lhs = (char *)name;
    char *rhs = (char *)entry_name;

    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS)
    {
        lhs++;
        rhs++;
    }

    if (*rhs)
        return false;
    if (*lhs && !IS_SEPARATOR(*lhs))
        return false;
    if (IS_SEPARATOR(*lhs))
        lhs++;
    *next = lhs;
    return true;
}

// 获取 dir 目录下的 name 目录所在的 dentry_t 和 buffer_t
static buffer_t *find_entry(
    inode_t **dir,
    const char *name,
    char **next,
    dentry_t **result)
{
    // 确保为目录
    assert(ISDIR((*dir)->desc->mode));

    // dir 目录最多子目录数量
    u32 entries = (*dir)->desc->size / sizeof(dentry_t);

    index_t i = 0;
    index_t block = 0;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    index_t nr = EOF;

    for (; i < entries; i ++, entry++)
    {
        // buf 未初始化或当前 entry 超出 buf 所在块
        // 则更新 buf
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE)
        {
            brelse(buf);
            block = bmap((*dir), i / BLOCK_DENTRIES, false);

            assert(block);

            buf = bread((*dir)->dev, block);
            entry = (dentry_t *)buf->data;
        }

        if (match_name(name, entry->name, next))
        {
            *result = entry;
            return buf;
        }
    }

    brelse(buf);
    return NULL;
}

static buffer_t *add_enrtry(inode_t *dir, const char *name, dentry_t **result)
{
    char *next = NULL;

    buffer_t *buf = find_entry(&dir, name, &next, result);

    if (buf)
    {
        return buf;
    }

    // name 中不能有分隔符
    for (size_t i = 0; i < NAME_LEN && name[i]; i ++)
    {
        assert(!IS_SEPARATOR(name[i]));
    }

    index_t i = 0;
    index_t block = 0;
    dentry_t *entry;

    for (; true; i ++, entry++)
    {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE)
        {
            brelse(buf);
            block = bmap(dir, i / BLOCK_DENTRIES, true);
            assert(block);

            buf = bread(dir->dev, block);
            entry = (dentry_t *)buf->data;
        }

        if (i * sizeof(dentry_t) >= dir->desc->size)
        {
            entry->nr = 0;
            dir->desc->size = (i + 1) * sizeof(dentry_t);
            dir->buf->dirty = true;
        }

        if (entry->nr)
            continue;
        
        strncpy(entry->name, name, NAME_LEN);
        buf->dirty = true;
        dir->desc->mtime = time();
        dir->buf->dirty = true;
        *result = entry;
        return buf;
    }
}

#include <yui/task.h>

void dir_test()
{
    task_t *task = running_task();
    inode_t *inode = task->iroot;
    inode->count ++;
    char *next = NULL;
    dentry_t *entry = NULL;
    buffer_t *buf = NULL;

    buf = find_entry(&inode, "hello.txt", &next, &entry);
    index_t nr = entry->nr;
    brelse(buf);

    buf = add_enrtry(inode, "world.txt", &entry);
    entry->nr = nr;
    inode_t *hello = iget(inode->dev, nr);
    hello->desc->nlinks++;
    hello->buf->dirty = true;

    iput(inode);
    iput(hello);
    brelse(buf);
}