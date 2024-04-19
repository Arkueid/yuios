extern void console_init();
extern void gdt_init();
extern void interrupt_init();
extern void task_init();
extern void hang();
extern void printk();
extern void clock_init();
extern void time_init();

void kernel_init()
{
    console_init();
    gdt_init();
    interrupt_init();
    printk("Hello, Yui\n");
    // task_init();
    clock_init();

    asm volatile("sti"); // 开中断
    hang();
}
