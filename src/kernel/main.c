#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>
#include <yui/console.h>
#include <yui/printk.h>
#include <yui/assert.h>
#include <yui/debugk.h>
#include <yui/global.h>

void kernel_init()
{
    console_init();

    gdt_init();
    
    return;
}
