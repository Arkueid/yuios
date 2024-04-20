#include <yui/memory.h>
#include <yui/types.h>
#include <yui/debug.h>
#include <yui/assert.h>
#include <yui/yui.h>

#define ZONE_VALID 1    // ards 可用区域
#define ZONE_RESERVED 2 // ards 不可用区域

// 页大小为 0x1000
#define IDX(addr) ((u32)addr >> 12) // 获取 addr 的页索引

typedef struct ards_t
{
    u64 base; // 内存基地址
    u64 size; // 内存长度
    u32 type; // 类型
} _packed ards_t;

u32 memory_base = 0; // 可用内存基地址
u32 memory_size = 0; // 可用内存大小
u32 total_pages = 0; // 所有内存页数
u32 free_pages = 0;  // 空闲内存页数

#define used_pages (total_pages - free_pages) // 已用页数

void memory_init(u32 magic, u32 addr)
{
    u32 count;
    ards_t *ptr;

    // 如果是通过 yui loader 进入的内核
    if (magic == YUI_MAGIC)
    {
        count = *(u32 *)addr;
        ptr = (ards_t *)(addr + 4);
        for (size_t i = 0; i < count; i++, ptr++)
        {
            DEBUG("Memory base 0x%p size 0x%p type %d\n",
                  (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size)
            {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    }
    else
    {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    DEBUG("ARDS count %d\n", count);
    DEBUG("Memory base 0x%p\n", (u32)memory_base);
    DEBUG("Memory size 0x%p\n", (u32)memory_size);

    assert(memory_base == MEMORY_BASE); // 内存开始的位置为 1M
    assert((memory_size & 0xfff) == 0); // 要求按页对齐

    // 总页数，包括预先占用的
    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    DEBUG("Total pages %d\n", total_pages);
    DEBUG("Free pages %d\n", free_pages);
}