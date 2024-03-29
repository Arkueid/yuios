#include <yui/types.h>
#include <stdio.h>

typedef struct descriptor
{
    unsigned short limit_low;
    unsigned int base_low : 24;
    unsigned char type : 4;
    unsigned char segment : 1;
    unsigned char DPL : 2;
    unsigned char present : 1;
    unsigned char limit_high : 4;
    unsigned char available : 1;
    unsigned char long_mode : 1;
    unsigned char big : 1;
    unsigned char granularity : 1;
    unsigned char base_high;
} _packed descriptor;


int main() 
{
    printf("size of u8 %d\n", sizeof(u8));
    printf("size of u32 %d\n", sizeof(u16));
    printf("size of u32 %d\n", sizeof(u32));
    printf("size of u64 %d\n", sizeof(u64));
    printf("size of descriptor %d\n", sizeof(descriptor));

    descriptor des;
    return 0;
}