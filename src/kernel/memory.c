#include <yui/yui.h>
#include <yui/memory.h>
#include <yui/types.h>
#include <yui/debug.h>
#include <yui/assert.h>
#include <yui/stdlib.h>
#include <yui/string.h>

#define ZONE_VALID 1    // ards 可用区域
#define ZONE_RESERVED 2 // ards 不可用区域

// 页大小为 0x1000
#define IDX(addr) ((u32)addr >> 12)            // 获取 addr 的页索引
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)           // 获取页目录索引
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // 获取页表索引
#define PAGE(idx) ((u32)idx << 12)             // 获取页索引 idx 对应页的内存开始位置

#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

// 在内存中找两个页，存放页表和内核页
// 内核页目录
#define KERNEL_PAGE_DIR 0x1000

// 内核使用的页表
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
};

// 一个页表映射4M空间，总共两个页表
// 这个写法只是最终结果正确
#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

typedef struct ards_t
{
    u64 base; // 内存基地址
    u64 size; // 内存长度
    u32 type; // 类型
} _packed ards_t;

static u32 memory_base = 0; // 可用内存基地址
static u32 memory_size = 0; // 可用内存大小
static u32 total_pages = 0; // 所有内存页数
static u32 free_pages = 0;  // 空闲内存页数

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

    if (memory_size < KERNEL_MEMORY_SIZE)
    {
        panic("System memory is %dM too small, at least %dM needed\n",
              memory_size / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static u32 start_page = 0;   // 可分配物理内存起始位置
static u8 *memory_map;       // 物理内存数组
static u32 memory_map_pages; // 物理内存数组占用的页数

void memory_map_init()
{
    // 初始化物理内存数组
    memory_map = (u8 *)memory_base;

    // 计算物理内存数组占用的页数，一页用一个字节来管理
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);

    free_pages -= memory_map_pages;

    // 初始化内存数组为0
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    // 前 1M 的内存位置 以及 物理内存数组已占用的页
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    for (size_t i = 0; i < start_page; i++)
    {
        // 内存数组对应的内存不能使用，引用置1
        memory_map[i] = 1;
    }

    DEBUG("Total pages %d free pages %d\n", total_pages, free_pages);
}

static u32 get_page()
{
    for (size_t i = start_page; i < total_pages; i++)
    {
        if (!memory_map[i]) // 有可用内存就分配
        {
            memory_map[i] = 1;
            free_pages--;
            assert(free_pages >= 0);
            u32 page = ((u32)i) << 12;
            DEBUG("GET page 0x%p\n", page);
            return page; // 返回分配的页起始地址
        }
    }
    panic("Out of Memory!!!");
}

// 释放一页物理内存
static void put_page(u32 addr)
{
    ASSERT_PAGE(addr); // 传入的地址是页的起始地址，页大小的整数倍

    u32 idx = IDX(addr);

    assert(idx >= start_page && idx < total_pages);

    // @todo 保证至少有一个引用
    assert(memory_map[idx] >= 1);

    // 物理引用减一
    memory_map[idx]--;

    // 若为空，则空闲页+1
    if (!memory_map[idx])
    {
        free_pages++;
    }

    assert(free_pages > 0 && free_pages < total_pages);
    DEBUG("PUT page 0x%p\n", addr);
}

u32 _inline get_cr3()
{
    asm volatile("movl %cr3, %eax\n");
}

void set_cr3(u32 pde)
{
    ASSERT_PAGE(pde);
    // ::"a"(pde) 在执行该行语句前，将pde的值移动到eax中
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

static void _inline enable_page()
{
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0\n");
}

static void entry_init(page_entry_t *entry, u32 index)
{
    *(u32 *)entry = 0; // 先全部初始化为0
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

void mapping_init()
{
    // 页目录起始地址
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    // 初始化两个内核页表
    index_t index = 0;
    for (index_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++)
    {
        // 页表
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX((u32)pte));

        for (size_t tidx = 0; tidx < 1024; tidx++, index++)
        {
            // 第0页不映射，用于空指针实现、缺页等的实现
            if (index == 0)
                continue;

            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1;
        }
    }

    // 页目录最后一个页表指向页目录本身，方便修改页目录
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    // cr3寄存器配置页表
    set_cr3((u32)pde);

    // cr0寄存器开启分页
    enable_page();
}

static page_entry_t *get_pde()
{
    return (page_entry_t *)(0xfffff000);
}

static page_entry_t *get_pte(u32 vaddr)
{
    return (page_entry_t *)(0xffc00000 | (DIDX(vaddr)) << 12);
}


// 刷新快表
static void flush_tlb(u32 vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

void memory_test()
{
    
    BMB;
    
    u32 vaddr = 0x4000000; // 虚拟地址
    u32 paddr = 0x1400000;  // 物理地址
    u32 table = 0x900000;  // 页表地址

    // 内存从高地址开始
    // 0xfffff000
    page_entry_t *pde = get_pde();

    // 高10位获取页目录 表项
    // 0xfffff010
    page_entry_t *dentry = &pde[DIDX(vaddr)];
    entry_init(dentry, IDX(table));

    // 0xffc04000
    page_entry_t *pte = get_pte(vaddr);

    page_entry_t *tentry = &pte[TIDX(vaddr)];

    entry_init(tentry, IDX(paddr));

    BMB;

    char *ptr = (char*) (vaddr);
    ptr[0] = 'a';

    entry_init(tentry, IDX(0x1500000));
    flush_tlb(vaddr);

    BMB;

    ptr[0] = 'b';

    BMB;
}