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


void kernel_init()
{
    console_init();
    gdt_init();
    interrupt_init();
        
    printk("Hello, Yui~\n");
    return;
}
