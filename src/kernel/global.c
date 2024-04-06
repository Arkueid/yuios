#include <yui/global.h>
#include <yui/string.h>
#include <yui/debug.h>

// 全局描述符表
descriptor_t gdt[GDT_SIZE];
// 全局描述符指针
pointer_t gdt_ptr;

void gdt_init()
{
    asm volatile("sgdt gdt_ptr");

    memcpy(&gdt, (void *)gdt_ptr.base, gdt_ptr.limit + 1);

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;

    asm volatile("lgdt gdt_ptr\n");
}
