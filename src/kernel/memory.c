#include <yui/yui.h>
#include <yui/memory.h>
#include <yui/types.h>
#include <yui/debug.h>
#include <yui/assert.h>
#include <yui/stdlib.h>
#include <yui/string.h>
#include <yui/bitmap.h>
#include <yui/multiboot2.h>
#include <yui/task.h>

#define ZONE_VALID 1    // ards 可用区域
#define ZONE_RESERVED 2 // ards 不可用区域

// 页大小为 0x1000
#define IDX(addr) ((u32)addr >> 12)            // 页框地址转页索引
#define PAGE(idx) ((u32)idx << 12)             // 页索引转页框地址
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff) // 获取页目录索引
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // 获取页表索引

#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

#define PDE_MASK 0xffc00000

// 内核位图数据存放位置
#define KERNEL_MAP_BITS 0x4000


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

bitmap_t kernel_bitmap;

#define used_pages (total_pages - free_pages) // 已用页数

static u32 start_page = 0;   // 可分配物理内存起始位置
static u8 *memory_map;       // 物理内存数组
static u32 memory_map_pages; // 物理内存数组占用的页数

void memory_init(u32 magic, u32 addr)
{
    u32 count = 0;

    // 如果是通过 yui loader 进入的内核
    if (magic == YUI_MAGIC)
    {
        count = *(u32 *)addr;
        ards_t *ptr = (ards_t *)(addr + 4);

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
    else if (magic == MULTIBOOT2_MAGIC)
    {
        u32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        DEBUG("Announced mbi size 0x%x\n", size);

        while (tag->type != MULTIBOOT_TAG_TYPE_END)
        {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
                break;

            tag = (multi_tag_t *)((u32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
        multi_mmap_entry_t *entry = mtag->entries;
        while ((u32)entry < (u32)tag + tag->size)
        {
            DEBUG("Memory base 0x%p size 0x%p type %d\n", (u32)entry->addr, (u32)entry->len, (u32)entry->type);

            count++;
            if (entry->type == ZONE_VALID && entry->len > memory_size)
            {
                memory_base = (u32)entry->addr;
                memory_size = (u32)entry->len;
            }
            entry = (multi_mmap_entry_t *)((u32)entry + mtag->entry_size);
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

    u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8;
    bitmap_init(&kernel_bitmap, (char *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_bitmap, memory_map_pages);
}

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
            u32 page = PAGE(i);
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

u32 get_cr3()
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

static page_entry_t *get_pte(u32 vaddr, bool create)
{
    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || entry->present);

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    if (!entry->present)
    {
        DEBUG("Get and create page table entry for 0x%p\n", vaddr);

        u32 page = get_page();
        entry_init(entry, IDX(page));
        memset(table, 0, PAGE_SIZE);
    }

    return table;
}

// 刷新快表
static void flush_tlb(u32 vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

static u32 scan_page(bitmap_t *map, u32 count)
{
    assert(count > 0);
    index_t index = bitmap_scan(map, count);

    if (index == EOF)
    {
        panic("Scan Page failed!!!\n");
    }

    u32 addr = PAGE(index);

    DEBUG("Scan page 0x%p count %d\n", addr, count);
    return addr;
}

static void reset_page(bitmap_t *map, u32 addr, u32 count)
{
    ASSERT_PAGE(addr);

    assert(count > 0);

    u32 index = IDX(addr);

    for (size_t i = 0; i < count; i++)
    {
        assert(bitmap_test(map, index + i));
        bitmap_set(map, index + i, 0);
    }
}

u32 alloc_kpage(u32 count)
{
    assert(count > 0);

    u32 vaddr = scan_page(&kernel_bitmap, count);
    DEBUG("ALLOC kernel pages 0x%p count %d\n", vaddr, count);
    return vaddr;
}

void free_kpage(u32 vaddr, u32 count)
{
    ASSERT_PAGE(vaddr);
    assert(count > 0);

    reset_page(&kernel_bitmap, vaddr, count);
    DEBUG("FREE kernel pages 0x%p count %d\n", vaddr, count);
}

void link_page(u32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    if (entry->present)
    {
        assert(bitmap_test(map, index));
        return;
    }

    assert(!bitmap_test(map, index));
    bitmap_set(map, index, true);

    u32 paddr = get_page();

    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);

    DEBUG("Link from 0x%p to 0x%p\n", vaddr, paddr);
}

void unlink_page(u32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    if (!entry->present)
    {
        assert(!bitmap_test(map, index));
        return;
    }

    assert(entry->present && bitmap_test(map, index));

    entry->present = false;
    bitmap_set(map, index, false);

    u32 paddr = PAGE(entry->index);

    DEBUG("Unlink from 0x%p to 0x%p\n", vaddr, paddr);

    if (memory_map[entry->index] == 1)
    {
        put_page(paddr);
    }
    flush_tlb(vaddr);
}
