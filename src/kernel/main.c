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
extern void syscall_init();
extern void sys_tests();
extern void keyboard_init();


#include <yui/types.h>
extern void set_interrupt_state(bool state);

void kernel_init()
{
    memory_map_init();
    mapping_init();
    interrupt_init();
    clock_init();
    // time_init();
    // rtc_init();

    task_init();
    syscall_init();

    keyboard_init();

    printk("Hello, Yui\n");

    set_interrupt_state(true);

    hang();
}
