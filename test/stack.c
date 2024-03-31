#include <stdio.h>

int main()
{
    char arr[] = {'H', 'e', 'l', 'l', 'o'};
    int * ptr = (int*) arr;
    ptr += sizeof(char*);
    // int *p = (int *)ptr;
    printf("%c\n", *((char*)ptr));
    return 0;
}