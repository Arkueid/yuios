extern void interrupt_init();
extern void task_init();
extern void hang();
extern void printk();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void set_alarm();
extern void memory_map_init();
extern void memory_test();

void kernel_init()
{
    memory_map_init();
    interrupt_init();

    // printk("Hello, Yui\n");
    
    // task_init();
    // clock_init();
    // time_init();
    // rtc_init();
    memory_test();

    asm volatile("sti"); // 开中断
    hang();
}
