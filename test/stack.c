#include <stdio.h>
#define ADD(exp) if (exp) printf("'%s' is ok\n", #exp); else printf("'%s' is shit\n", #exp);

int main()
{
    int left = 16;
    int zeropad = 1;
    ADD(left < zeropad);
    printf("%d", __LINE__);
    return 0;
}