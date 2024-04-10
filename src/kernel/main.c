#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>
#include <yui/console.h>
#include <yui/printk.h>
#include <yui/assert.h>
#include <yui/debug.h>
#include <yui/global.h>
#include <yui/task.h>
#include <yui/interrupt.h>
#include <yui/stdlib.h>

void kernel_init()
{
    console_init();
    gdt_init();
    interrupt_init();

    // 开中断
    asm volatile(
        "sti\n"
        "movl %eax, %eax\n");

    printk("Hello, Yui~\n");

    u32 counter = 0;
    while (true)
    {
        DEBUG("looping in kernel init %d...\n", counter++);
        delay(1000000000);
    }

    return;
}
