#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>
#include <yui/console.h>

char message[] = "Hello, world!!!\n";
char buf[1024];

void kernel_init()
{
    console_init();

    u32 count = 0;
    while (true)
    {
        buf[0] = 48 + (count % 10);
        u32 c = 1;
        console_write(buf, c);
        console_write(message, sizeof(message) - 1);
        count++;
    }
    return;
}
