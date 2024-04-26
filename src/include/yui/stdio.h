#ifndef YUI_STDIO_H
#define YUI_STDIO_H

#include <yui/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif