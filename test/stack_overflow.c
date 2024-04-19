// 栈溢出攻击
#include <stdio.h>
#include <string.h>
#include <memory.h>

// 0x5655618d
void function1()
{
    printf("success\n");
}

// 0x565561b8
void t(int a, int b)
{
    char x[10];
    // x自身、ebp、返回地址eip、参数a、参数b
    *(x+12+1) = (void*)(((char*)&t)-15);
    printf("%d\n", a);
    printf("%x %x\n", &function1, function1);
    return;
}

int main()
{
    int a=1, b=2;
    t(a, b);
    return 0;
}