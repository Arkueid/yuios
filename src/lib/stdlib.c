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


// bcd码转整数
void bcd_to_bin(u8 value)
{
    return (value & 0xf) + (value >> 4) * 10;
}

// 整数转bcd
void bin_to_bcd(u8 value)
{
    return (value / 10) * 0x10 + (value % 10); 
}