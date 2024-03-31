#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>
#include <yui/console.h>
#include <yui/stdarg.h>

void test_args(int cnt, ...)
{
    va_list args;
    va_start(args, cnt);

    int arg;
    while (cnt--)
    {
        arg = va_arg(args, char);
    }
    va_end(args);
}

void kernel_init()
{
    console_init();

    test_args(5, 'h', 'e', 'l', 'l', 'o');
    return;
}
