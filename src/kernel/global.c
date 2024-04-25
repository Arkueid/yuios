#include <yui/global.h>
#include <yui/string.h>
#include <yui/debug.h>

// 全局描述符表
descriptor_t gdt[GDT_SIZE];
// 全局描述符指针
pointer_t gdt_ptr;

void descriptor_init(descriptor_t *desc, u32 base, u32 limit)
{
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
}

void gdt_init()
{
    DEBUG("init gdt!!!\n");

    memset(gdt, 0, sizeof(gdt));

    descriptor_t *desc;
    desc = gdt + KERNEL_CODE_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;
    desc->granularity = 1;
    desc->big = 1;
    desc->long_mode = 0;
    desc->present = 1;
    desc->DPL = 0;
    desc->type = 0b1010;

    desc = gdt + KERNEL_DATA_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;     // 数据段
    desc->granularity = 1; // 4k
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是64位
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 内核特权级
    desc->type = 0b0010;   // 数据/向上增长/可写/没有被访问过

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
}
