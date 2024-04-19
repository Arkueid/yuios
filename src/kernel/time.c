#include <yui/time.h>
#include <yui/debug.h>
#include <yui/stdlib.h>
#include <yui/io.h>
#include <yui/rtc.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define CMOS_SECOND 0x00
#define CMOS_MINUTE 0x02
#define CMOS_HOUR 0x04
#define CMOS_WEEKDAY 0x06
#define CMOS_DAY 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09
#define CMOS_CENTURY 0x032
#define CMOS_NMI 0x80

#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)

// 每个月开始时已经过去的天数
static int month[13] = {
    0, // 占位
    0, // 1月
    (31),
    (31 + 29),
    (31 + 29 + 31),
    (31 + 29 + 31 + 30),
    (31 + 29 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 30 + 31 + 30),
};

time_t startup_time;
int century;

// 计算时间戳
time_t mktime(tm *time)
{
    time_t res;
    int year;                // 距 1970 年过去多少年
    if (time->tm_year >= 70) // tm_year 从 1900 开始算起
        year = time->tm_year - 70;
    else
        year = time->tm_year - 70 + 100;

    res = YEAR * year;

    // 已经过去的闰年数
    res += DAY * ((year + 1) / 4);

    res += month[time->tm_mon] * DAY;

    if (time->tm_mon > 2 && (year + 2) % 4)
        res -= DAY;

    // 这个月已经过去的天数
    res += DAY * (time->tm_mday - 1);

    res += HOUR * time->tm_hour;

    // 这个分钟过去的秒
    res += time->tm_sec;
    return res;
}

void time_read_bcd(tm *time)
{
    // 访问CMOS的速度很慢，为了减小时间误差在读取了所有数值后
    // 对秒数进行校对，使误差小于等于1s
    do
    {
        time->tm_sec = cmos_read(CMOS_SECOND);
        time->tm_min = cmos_read(CMOS_MINUTE);
        time->tm_hour = cmos_read(CMOS_HOUR);
        time->tm_mday = cmos_read(CMOS_DAY);
        time->tm_mon = cmos_read(CMOS_MONTH);
        time->tm_wday = cmos_read(CMOS_WEEKDAY);
        time->tm_year = cmos_read(CMOS_YEAR);
        century = cmos_read(CMOS_CENTURY);

    } while (time->tm_sec != cmos_read(CMOS_SECOND));
}

int get_yday(tm *time)
{
    int res = month[time->tm_mon];
    res += time->tm_mday;

    int year;
    if (time->tm_year >= 70)
        year = time->tm_year - 70;
    else
        year = time->tm_year - 70 + 100;

    if ((year + 2) % 4 && time->tm_mon > 2)
    {
        res -= 1;
    }
    return res;
}

void time_read(tm *time)
{
    time_read_bcd(time);

    time->tm_sec = bcd_to_bin(time->tm_sec);
    time->tm_min = bcd_to_bin(time->tm_min);
    time->tm_hour = bcd_to_bin(time->tm_hour);
    time->tm_mday = bcd_to_bin(time->tm_mday);
    time->tm_mon = bcd_to_bin(time->tm_mon);
    time->tm_wday = bcd_to_bin(time->tm_wday);
    time->tm_year = bcd_to_bin(time->tm_year);
    time->tm_yday = get_yday(time);
    time->tm_isdst = -1;
    century = bcd_to_bin(century);
}

void time_init()
{
    tm time;
    time_read(&time);
    startup_time = mktime(&time);
    DEBUG("startup time: %d%d-%02d-%02d %02d:%02d:%02d\n",
          century,
          time.tm_year,
          time.tm_mon,
          time.tm_mday,
          time.tm_hour,
          time.tm_min,
          time.tm_sec);
}
