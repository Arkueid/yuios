#include <yui/bitmap.h>
#include <yui/string.h>
#include <yui/yui.h>
#include <yui/assert.h>

// 构造位图
void bitmap_make(bitmap_t *map, char *bits, u32 length, u32 offset)
{
    map->bits = bits;
    map->length = length;
    map->offset = offset;
}

// 位图初始化
void bitmap_init(bitmap_t *map, char *bits, u32 length, u32 start)
{
    memset(bits, 0, length);
    bitmap_make(map, bits, length, start);
}

// 测试某一位是否为0
bool bitmap_test(bitmap_t *map, index_t index)
{
    assert(index >= map->offset);

    // 得到位图的索引
    index_t idx = index - map->offset;

    // 索引所在字节位置
    u32 bytes = idx / 8;

    // 一个字节中的位置
    u8 bits = idx % 8;

    assert(bytes < map->length);

    return (map->bits[bytes] & (1 << bits));
}

void bitmap_set(bitmap_t *map, index_t index, bool value)
{
    assert(value == 0 || value == 1);

    assert(index >= map->offset);

    u32 bytes = index / 8;
    u8 bits = index % 8;

    assert(bytes < map->length);

    if (value)
    {
        map->bits[bytes] |= (1 << bits);
    }
    else
    {
        map->bits[bytes] &= ~(1 << bits);
    }
}

// 从 bitmap 中查找连续的 count 位
// 并使用这些位对应的页
int bitmap_scan(bitmap_t *map, u32 count)
{
    int start = EOF;                 // 标记开始的位置
    u32 bits_left = map->length * 8; // 剩余的位数
    u32 next_bit = 0;                // 下一个位
    u32 counter = 0;                 // 计数器

    // 从头开始找
    while (bits_left-- > 0)
    {
        if (!bitmap_test(map, map->offset + next_bit))
        {
            counter++;
        }
        else
        {
            counter = 0;
        }

        next_bit++;

        if (counter == count)
        {
            start = next_bit - count;
            break;
        }
    }

    bits_left = count;
    next_bit = start;

    if (start != EOF)
    {
        while (bits_left--)
        {
            bitmap_set(map, next_bit, true);
            next_bit ++;
        }
    }

    return start + map->offset;
}

// 位图测试
#include <yui/debug.h>

#define LEN 2
u8 buf[LEN];
bitmap_t map;

void bitmap_tests()
{
    bitmap_init(&map, buf, LEN, 0);
    for (size_t i = 0; i < 33; i ++)
    {
        index_t idx = bitmap_scan(&map, 1);
        if (idx == EOF)
        {
            DEBUG("bitmap test finished\n");
            break;
        }
        DEBUG("%d\n", idx);
    }
}