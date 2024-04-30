#include <yui/string.h>

// 复制字符串
char *strcpy(char *dest, const char *src)
{
    char *ptr = dest;
    while (true)
    {
        *ptr++ = *src; // 先自增，再解引用
        if (*src++ == EOS)
        {
            return dest;
        }
    }
}

char *strncpy(char *dest, const char *src, size_t count)
{
    char *ptr = dest;
    size_t nr = 0;
    for (; nr < count; nr++)
    {
        *ptr++ = *src;
        // 当前字符是 end of string '\0'
        if (*src++ == EOS)
            return dest;
    }
    dest[count - 1] = EOS;
    return dest;
}

// 字符串拼接，可能会有问题？
char *strcat(char *dest, const char *src)
{
    char *ptr = dest;
    while (*ptr != EOS)
    {
        ptr++;
    }
    while (true)
    {
        *ptr++ = *src;
        if (*src++ == EOS)
        {
            return dest;
        }
    }
}

// 获取字符串长度
size_t strlen(const char *str)
{
    char *ptr = (char *)str;
    while (*ptr != EOS)
        ptr++;
    return ptr - str;
}

// 比较字符串
int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS)
    {
        lhs++;
        rhs++;
    }
    return *lhs < *rhs ? -1 : *lhs > *rhs;
}

// 查找字符的指针
char *strchr(const char *str, int ch)
{
    char *ptr = (char *)str;
    while (true)
    {
        if (*ptr++ == ch)
        {
            return ptr;
        }
        if (*ptr == EOS)
        {
            return NULL;
        }
    }
}

// 从右往左查找，找到则返回字符的指针
char *strrchr(const char *str, int ch)
{
    char *last = NULL;
    char *ptr = (char *)str;
    while (true)
    {
        if (*ptr == ch)
        {
            last = ptr;
        }
        if (*ptr++ == EOS)
        {
            return last;
        }
    }
}

// 比较两段连续内存，可能会有问题
int memcmp(const void *lhs, const void *rhs, size_t count)
{
    char *lptr = (char *)lhs;
    char *rptr = (char *)rhs;
    while (*lptr == *rptr && count-- > 0)
    {
        lptr++;
        rptr++;
    }
    return *lptr < *rptr ? -1 : *lptr > *rptr;
}

// 初始化连续内存
void *memset(void *dest, int ch, size_t count)
{
    char *ptr = dest;
    while (count--)
    {
        *ptr++ = ch;
    }
    return dest;
}

// 复制内存
void *memcpy(void *dest, const void *src, size_t count)
{
    char *dptr = dest;
    char *sptr = (char*)src;
    while (count--)
    {
        *dptr++ = *sptr++;
    }
    return dest;
}

// 查找
void *memchr(const void *ptr, int ch, size_t count)
{
    char *tptr = (char *)ptr;
    while (count--)
    {
        if (*tptr == ch)
        {
            return tptr;
        }
        tptr++;
    }
    return NULL;
}
