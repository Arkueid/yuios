extern void console_init();
extern void gdt_init();
extern void interrupt_init();
extern void task_init();
extern void hang();
extern void printk();


void kernel_init()
{
    console_init();
    gdt_init();
    interrupt_init();
    // task_init();
    printk("Hello, Yui\n");

    asm volatile("sti"); // 开中断
    hang();
}
