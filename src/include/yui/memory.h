#ifndef YUI_MEMORY_H
#define YUI_MEMORY_H

#include <yui/types.h>

#define PAGE_SIZE 0x1000     // 一页的大小
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置

typedef struct page_entry_t
{
    u8 present : 1;  // 在内存中
    u8 write : 1;    // 是否可写
    u8 user : 1;     // 1 所有人，0 超级用户 DPL < 3
    u8 pwt : 1;      // page write through 1 直写模式，0 回写模式
    u8 pcd : 1;      // page cache disable
    u8 accessed : 1; // 被访问过，用于统计使用频率
    u8 dirty : 1;    // 脏页，表示该页缓冲被修改过
    u8 pat : 1;      // page attribute table 页大小，4k / 4M
    u8 global : 1;   // 全局，所有进程都使用
    u8 ignored : 3;  // 未使用
    u32 index : 20;  // 页框索引
} _packed page_entry_t;

u32 get_cr3();
void set_cr3(u32 pde);

#endif