#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>
#include <yui/console.h>
#include <yui/stdarg.h>
#include <yui/printk.h>


void kernel_init()
{
    console_init();

    int cnt = 30;
    while (cnt--)
    {
        printk("Hello %s\n", "Yui~");
    }
    
    return;
}
