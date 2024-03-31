#ifndef YUI_STDARG_H
#define YUI_STDARG_H

// 可变参数指针
typedef void *va_list;

// 根据函数中的第一个参数v获取可变长参数起始位置
#define va_start(ap, v) (ap = (va_list)&v + sizeof(v))
// 读取当前参数并指向下一个参数
#define va_arg(ap, t) (*(t *)((ap += sizeof(t *)) - sizeof(t *)))
// 结束可变长参数并赋值为空指针
#define va_end(ap) (ap = (va_list)0)

#endif