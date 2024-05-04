#ifndef YUI_TIME_H
#define YUI_TIME_H

#include <yui/types.h>

typedef struct tm
{
    int tm_sec;   // [0, 59]
    int tm_min;   // [0, 59]
    int tm_hour;  // [0, 59]
    int tm_mday;  // [0, 31]
    int tm_mon;   // [0, 11]
    int tm_year;  // 从 1900 开始的年数
    int tm_wday;  // [0, 6]
    int tm_yday;  // [0, 365]
    int tm_isdst; // 夏令时标志
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time);
void localtime(time_t stamp, tm *time);

#endif