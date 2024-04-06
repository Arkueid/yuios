#include <yui/interrupt.h>
#include <yui/global.h>
#include <yui/debug.h>
#include <yui/printk.h>

#define ENTRY_SIZE 0x20 // 定义32个中断处理函数入口

gate_t idt[IDT_SIZE];
pointer_t idt_ptr;

handler_t handler_table[IDT_SIZE];

// 中断函数入口地址
extern handler_t handler_entry_table[ENTRY_SIZE];

static char *messages[] = {
    "#DE\0",
    "#DB\0",
    "#NMI\0",
    "#BP\0",
    "#OF\0",
    "#BR\0",
    "#UD\0",
    "#NM\0",
    "#DF\0",
    "#CSO\0",
    "#TS\0",
    "#NP\0",
    "#SS\0",
    "#GP\0",
    "#PF\0",
    "#Reserved\0",
    "#MF\0",
    "#AC\0",
    "#MC\0",
    "#XM/#XF\0",
    "#VE\0",
    "#CP\0",
};

void exception_handler(int vector)
{
    char *message = NULL;
    if (vector < 22)
    {
        message = messages[vector];
    }
    else
    {
        message = messages[15];
    }

    printk("Exception: [0x%02x] %s \n", vector, message);

    while (true)
        ;
}

void interrupt_init()
{
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        gate_t *gate = &idt[i];
        handler_t handler = handler_entry_table[i];

        gate->offset0 = (u32)handler & 0xffff;
        gate->offset1 = ((u32)handler >> 16) & 0xffff;
        gate->selector = 1 << 3; // 代码段
        gate->reserved = 0;
        gate->type = 0b1110; // 中断门
        gate->segment = 0;   // 系统段
        gate->DPL = 0;       // 内核态
        gate->present = 1;   // 有效
    }

    for (int i = 0; i < IDT_SIZE; i++)
    {
        handler_table[i] = exception_handler;
    }

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;

    asm volatile("lidt idt_ptr\n");
}