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

// 计算 num 分成 size
u32 div_round_up(u32 num, u32 size)
{   
    // 向上取整
    return (num + size -1) / size;
}

int atoi(const char *str)
{
    if (str == NULL)
        return 0;
    bool sign = 1;
    int result = 0;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    for (; *str; str++)
    {
        result = result * 10 + (*str - '0');
    }
    return result * sign;
}