extern void interrupt_init();
extern void task_init();
extern void hang();
extern void printk();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void set_alarm();
extern void memory_map_init();
extern void mapping_init();

#include <yui/types.h>

extern bool interrupt_disable();
extern void set_interrupt_state(bool state);

void interrupt_test()
{
    // 关中断
    bool state = interrupt_disable();

    // critical segment

    // 恢复中断状态
    set_interrupt_state(state);
}

void kernel_init()
{
    memory_map_init();
    mapping_init();
    interrupt_init();

    // task_init();
    // clock_init();
    // time_init();
    // rtc_init();
    asm volatile("sti"); // 开中断

    interrupt_test();

    printk("Hello, Yui\n");

    hang();
}
