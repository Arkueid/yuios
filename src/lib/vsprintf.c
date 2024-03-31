#include <yui/stdarg.h>
#include <yui/string.h>

#define ZEROPAD 1  // 填充0
#define SIGN 2     // unsigned / signed
#define PLUS 4     // 显示加号
#define SPACE 8    // 显示空格
#define LEFT 16    // 左调整
#define SPECIAL 32 // 0x
#define SMALL 64   // 使用小写字母

#define is_digit(c) ((c) >= '0' && (c) <= '9')

// 双重指针，不能改变源字符串的指针
// 将字符数字字符串转换为整数，并将指针前移
static int skip_atoi(const char **s)
{
    int i = 0;
    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

/**
 * @param str   存放结果字符串
 * @param num   待转换数字
 * @param base  基数
 * @param size  字符串长度
 * @param precision 输出数字的位数？
 * @param flags 输出控制标志
 */
static char *number(char *str, unsigned long num, int base, int size, int precision, int flags)
{
    // 填充字符，符号，36位，一个32位数字，最多两位进制标识，1位符号，最后一位结束符
    char c, sign, tmp[36];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    int index;
    char *ptr = str;

    // 如果左对齐，则屏蔽填0标志
    if (flags & LEFT)
        flags &= ~ZEROPAD;

    // 如果进制基数小于 2 大于 36 则退出处理
    if (base < 2 || base > 36)
        return 0;

    // 填充符
    c = (flags & ZEROPAD) ? '0' : ' ';

    // 数字小于0，符号置为'-'，并取绝对值
    if (flags & SIGN && num < 0)
    {
        sign = '-';
        num = -num;
    }
    else // 显示加号或显示空格
        sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);

    // 如果有符号，从size中减去符号位
    if (sign)
        size--;

    // 如果有进制转换，则显示进制标识
    if (flags & SPECIAL)
    {
        if (base == 16)
            size -= 2; // 0x前缀
        else if (base == 8)
            size--; // 0前缀
    }

    // 将数字绝对值转换为字符串tmp
    i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
        while (num != 0)
        {
            index = num % base;
            num /= base;
            // 倒序数字
            tmp[i++] = digits[index];
        }

    // 如果输出的字符串长度大于所给的精度，则拓展精度为输出字符串的长度
    if (i > precision)
        precision = i;

    // 减去字符串长度
    size -= precision;

    // 如果是0填充，则0填充在符号后，如果是空格填充，则空格填充在符号之前
    // 不是左对齐或零填充的情况下，若长度有剩余，则填充空格
    if (!(flags & (ZEROPAD + LEFT)))
        while (size-- > 0)
            *str++ = ' ';

    // 添加符号
    if (sign)
        *str++ = sign;

    // 填充进制标识
    if (flags & SPECIAL)
    {
        if (base == 8)
            *str++ = '0';
        else if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33]; // x
        }
    }

    // 不是左对齐，且没有空格填充时（即size没有减为0），则填充0
    if (!(flags & LEFT))
        while (size-- > 0)
            *str++ = c;

    // 数字位数小于精度，则填充0
    while (i < precision--)
        *str++ = '0';

    // 填充数字
    while (i-- > 0)
        *str++ = tmp[i];

    // size 仍有剩余，说明是左对齐，空格填充，继续填充剩余字符串
    while (size-- > 0)
        *str++ = ' ';

    return str;
}

/**
 * @brief
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
    int len;
    int i;

    char *str;
    char *s;
    int *ip;

    int flags;

    int field_width;
    int precision;
    int qualifier;

    for (str = buf; *fmt; ++fmt)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
        flags = 0;

    repeat:
        ++fmt;
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        field_width = -1;

        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);

        // 当前为*，则下一个参数指定宽度
        else if (*fmt == '*')
        {
            ++fmt;
            field_width = va_arg(args, int);

            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        precision = -1;

        // 当前字符为.，则下一个参数表示精度
        if (*fmt == '.')
        {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            // 当前字符为*（占位符），则下一个参数表示精度
            else if (*fmt == '*')
            {
                precision = va_arg(args, int);
            }

            if (precision < 0)
                precision = 0;
        }

        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            ++fmt;
        }

        // 转换提示符
        switch (*fmt)
        {
        case 'c':
            if (!(flags & LEFT))
                while (!(flags & LEFT))
                    *str++ = ' ';
            *str++ = (unsigned char)va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            break;
        case 's':
            // 说明下一个参数是字符串指针
            s = va_arg(args, char *);
            len = strlen(s);
            if (precision < 0)
                precision = len;
            else if (len > precision)
                len = precision;
            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            break;
        case 'o':
            str = number(str, va_arg(args, unsigned long), 8,
                         field_width, precision, flags);
            break;
        // 格式转换符是p，表示输出的是一个指针
        case 'p':
            if (field_width == -1)
            {
                field_width = 0;
                flags |= ZEROPAD;
            }
            str = number(str,
                         (unsigned long)va_arg(args, void *), 16,
                         field_width, precision, flags);
            break;
        case 'x':
            flags |= SMALL;
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16,
                         field_width, precision, flags);
            break;
        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10,
                         field_width, precision, flags);
            break;
        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;
        default:
            if (*fmt != '%')
                *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                --fmt;
            break;
        }
    }

    *str = '\0';

    return str - buf;
}

// 结果按格式输出字符串到 buf
int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(buf, fmt, args);
    va_end(args);
    return i;
}