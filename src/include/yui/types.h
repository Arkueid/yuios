#ifndef YUI_TYPES_H
#define YUI_TYPES_H

#define EOF -1
#define NULL ((void *)0)

#define EOS '\0' // 字符串结尾

#define CONCAT(X, Y) X##Y
#define RESERVED_TOKEN(X, Y) CONCAT(X, Y)
#define RESERVED RESERVED_TOKEN(reserved, __LINE__)

#ifndef __cplusplus
#define bool _Bool
#define true 1
#define false 0
#endif

// 用于定义特殊的结构体
#define _packed __attribute__((packed)) // 用于定义特殊的结构体

#define _inline __attribute__((always_inline)) inline
// 用于忽略函数的栈帧
#define _ofp __attribute__((optimize("omit-frame-pointer")))

typedef unsigned int size_t;

typedef char int8;
typedef int int32;
typedef long long int64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef int32 dev_t;

typedef u32 time_t;
typedef u32 index_t;

typedef int32 fd_t;
typedef enum std_fd_t
{
    STDIN_FILENO,
    STDOUT_FILENO,
    STDERR_FILENO,
} std_fd_t;

typedef int32 pid_t;

typedef u16 mode_t; // 文件权限

typedef int32 off_t;  // 文件偏移

// 标记弱符号
#define weak __attribute__((__weak__))

// 标记不会返回的函数
#define noreturn __attribute__((__noreturn__))

#endif
