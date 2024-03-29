#ifndef YUI_IO_H // 防止多次include导致重定义
#define YUI_IO_H // 头文件会在预处理阶段被展开到对应源文件

#include <yui/types.h>

extern u8 inb(u16 port);  // 输入一个字节
extern u16 inw(u16 port); // 输入一个字

extern void outb(u16 port, u8 value); // 输出一个字节
extern void outw(u16 port, u16 value);  // 输出一个字

#endif