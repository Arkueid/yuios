#include <stdio.h>

int main()
{
    int a[] = {1, 2, 3};
    int *p = a;
    *p++=3;
    printf("*p==%d\n", *a);
    return 0;
}