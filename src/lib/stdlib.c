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


// BCD码转整数
// 4位二进制数表示一个十进制位
// 如 26 -> 0010 0110
u8 bcd_to_bin(u8 value)
{
    return (value & 0xf) + (value >> 4) * 10;
}

// 整数转bcd
u8 bin_to_bcd(u8 value)
{
    return (value / 10) * 0x10 + (value % 10); 
}