#include <yui/stdlib.h>

void delay(u32 count)
{
    while (count--)
        ;
}

void hang() // 0x116cc
{
    while (true)
        ;
}