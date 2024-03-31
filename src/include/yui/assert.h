#ifndef YUI_ASSERT_H
#define YUI_ASSERT_H

/**
 * @param 表达式
 * @param 文件路径
 * @param 文件名称
 * @param 出错行号
 */
void assertion_failure(char *exp, char *file, char *base, int line);

// 断言
// 以下三个会在编译的时候被编译器替换
// __FILE__ 当前文件名称，包含路径？
// __BASE_FILE__ 文件名称
// __LINE__ 这个宏在一行，就会被替换为对应行号
#define assert(exp)                      \
    if (exp)                             \
        ;                                \
    else                                 \
        assertion_failure(#exp,          \
                          __FILE__,      \
                          __BASE_FILE__, \
                          __LINE__)

void panic(const char *fmt, ...);

#endif