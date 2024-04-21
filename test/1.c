

#include <stdio.h>

int get()
{
    int a = 123;
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n");
}

int main()
{
    int a = get();
    printf("%x\n", a);
    return 0;
}