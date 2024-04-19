#ifndef YUI_RTC_H
#define YUI_RTC_H

#include <yui/types.h>


// 读CMOS寄存器的值
u8 cmos_read(u8 addr);
// 写CMOS寄存器
void cmos_write(u8 addr, u8 value);

#endif