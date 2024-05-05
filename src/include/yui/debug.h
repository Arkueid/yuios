#ifndef YUI_DEBUG_H
#define YUI_DEBUG_H

void debugk(char *file, int line, const char *fmt, ...);

// Bochs Magic Breakpoint
#define BMB asm volatile("xchgw %bx, %bx"); 

// #define DEBUG(fmt, args...) NULL
#define DEBUG(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)

#endif