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
#define IDX(addr) ((u32)addr >> 12)            // 线性地址转页索引
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

    // 不经过 grub 进入内核
    if (magic == YUI_MAGIC)
    {
        count = *(u32 *)addr;
        ards_t *ptr = (ards_t *)(addr + 4);

        for (size_t i = 0; i < count; i++, ptr++)
        {
            DEBUG("Memory base 0x%p size 0x%p type %d\n",
                  (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            // 找到最后一个符合条件的内存范围
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
    assert((memory_size & 0xfff) == 0); // 页的整数倍，要求按页对齐

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

    // 初始化内核虚拟内存位图
    u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8; // 1字节 表示 8 位，位图按字节初始化
    bitmap_init(&kernel_bitmap, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_bitmap, memory_map_pages);
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

    // 保证至少有一个引用
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

u32 get_cr2()
{
    asm volatile("movl %cr2, %eax\n");
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
    // 指向自身，页目录既是目录页是页表
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    // cr3寄存器配置页表
    set_cr3((u32)pde);

    // cr0寄存器开启分页
    enable_page();
}

// 开启分页后访问 页目录
static page_entry_t *get_pde()
{
    // cr3 寄存器 页目录 的 物理地址 0x1000
    // 高10位 全1 0x3ff -> 最后一个页表（页表指向页目录自身0x1000）
    // 中间10位 全1 0x3ff -> 页目录自身又被看作为页表，它的最后一个页表项 还是指向自身
    // 低12位 000 页内偏移 0x1000
    // 最终该线性地址即为物理页 0x1000
    return (page_entry_t *)(0xfffff000); // 线性地址
}

// 获取页表
static page_entry_t *get_pte(u32 vaddr, bool create)
{
    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || entry->present); // 不创建的情况下页应该存在

    // 页表的虚拟地址
    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    if (!entry->present)
    {
        DEBUG("Get and create page table entry for 0x%p\n", vaddr);

        u32 page = get_page();
        // 页表所在页框号 x
        entry_init(entry, IDX(page));
        // 通过table找到 页框号x，并初始化这段页框
        memset(table, 0, PAGE_SIZE);
    }

    return table;
}

// 刷新快表
static void flush_tlb(u32 vaddr)
{
    // r 占位符
    // -> invlpg vaddr
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

static u32 scan_page(bitmap_t *map, u32 count)
{
    assert(count > 0);
    int32 index = bitmap_scan(map, count);

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

    put_page(paddr);

    flush_tlb(vaddr);
}

// 拷贝一页，返回物理地址
static u32 copy_page(void *page)
{
    u32 paddr = get_page();

    page_entry_t *entry = get_pte(0, false);
    entry_init(entry, IDX(paddr));
    memcpy((void *)0, (void *)page, PAGE_SIZE);

    entry->present = false;
    return paddr;
}

// 拷贝当前页目录
page_entry_t *copy_pde()
{
    task_t *task = running_task();
    page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
    memcpy(pde, (void *)task->pde, PAGE_SIZE);

    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(pde));

    page_entry_t *dentry;

    for (size_t didx = 2; didx < 1023; didx++)
    {
        dentry = &pde[didx];
        if (!dentry->present)
            continue;

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            entry = &pte[tidx];
            if (!entry->present)
                continue;

            // 至少有一个引用
            assert(memory_map[entry->index] > 0);

            // 只读
            entry->write = false;
            // 引用数 ++
            memory_map[entry->index]++;
            // 引用数不能超过255
            assert(memory_map[entry->index] < 255);
        }
        // 应该是写时 修改，
        u32 paddr = copy_page(pte);
        dentry->index = IDX(paddr);
    }

    set_cr3(task->pde);

    return pde;
}

void free_pde()
{
    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    page_entry_t *pde = get_pde();

    for (size_t didx = 2; didx < 1023; didx++)
    {
        page_entry_t *dentry = &pde[didx];
        if (!dentry->present)
        {
            continue;
        }
        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            page_entry_t *entry = &pte[tidx];
            if (!entry->present)
            {
                continue;
            }

            assert(memory_map[entry->index] > 0);
            put_page(PAGE(entry->index));
        }

        put_page(PAGE(dentry->index));
    }

    free_kpage(task->pde, 1);
    DEBUG("free pages %d\n", free_pages);
}

typedef struct page_error_code_t
{
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 reserved0 : 1;
    u8 fetch : 1;
    u8 protection : 1;
    u8 shadow : 1;
    u16 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
} _packed page_error_code_t;

void page_fault(
    u32 vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);

    u32 vaddr = get_cr2();
    DEBUG("fault address 0x%p\n", vaddr);

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = running_task();

    assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP);

    if (code->present)
    {
        assert(code->write);

        page_entry_t *pte = get_pte(vaddr, false);
        page_entry_t *entry = &pte[TIDX(vaddr)];

        assert(entry->present);
        assert(memory_map[entry->index] > 0);

        if (memory_map[entry->index] == 1)
        {
            entry->write = true;
            DEBUG("Write page for 0x%p\n", vaddr);
        }
        else
        {
            void *page = (void *)PAGE(IDX(vaddr));
            u32 paddr = copy_page(page);
            memory_map[entry->index]--;
            entry_init(entry, IDX(paddr));
            flush_tlb(vaddr);
            DEBUG("Copy page for 0x%p\n", vaddr);
        }
        return;
    }

    // 未映射物理页导致的异常
    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM))
    {
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        return;
    }

    panic("page fault!!!");
}

int32 sys_brk(void *addr)
{
    DEBUG("task brk 0x%p\n", addr);

    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);

    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    // TODO:上界后续修改 此处有问题
    assert(KERNEL_MEMORY_SIZE < brk || brk < USER_STACK_BOTTOM);

    u32 old_brk = task->brk;

    if (old_brk > brk)
    {
        for (u32 page = brk; brk < old_brk; page += PAGE_SIZE)
        {
            unlink_page(page);
        }
    }
    // 剩余物理页不足以
    else if (IDX(brk - old_brk) > free_pages)
    {
        // out of memory
        return -1;
    }
    // brk 以下的内存都是合法的，已分配
    // 实际使用时引发缺页异常动态分配

    task->brk = brk;
    return 0;
}