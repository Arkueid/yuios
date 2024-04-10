[bits 32]
; 中断程序入口

extern handler_table

section .text

%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
%ifn %2
    push 0x20222202
%endif
    push %1 ; 压入中断向量，跳转到中断入口
    jmp interrupt_entry
%endmacro

interrupt_entry:
    ; 保存上下文寄存器信息
    push ds
    push es
    push fs
    push gs
    pusha

    ; 4个段寄存器 和 8个 通用寄存器 eax, ebx, ecx, edx, ebp, esp, esi, edi
    mov eax, [esp + 12 * 4] ; 获取中断向量

    push eax

    call [handler_table + eax * 4]

    add esp, 4 ; pop eax

    popa

    pop gs
    pop fs
    pop es
    pop ds

    ; push %1
    ; error code
    add esp, 8 ; 出栈，中断返回
    iret


INTERRUPT_HANDLER 0x00, 0 ; 除零异常
INTERRUPT_HANDLER 0x01, 0 ; 调试
INTERRUPT_HANDLER 0x02, 0 ; 不可屏蔽中断
INTERRUPT_HANDLER 0x03, 0 ; 断点

INTERRUPT_HANDLER 0x04, 0 ; 溢出
INTERRUPT_HANDLER 0x05, 0 ; 越界
INTERRUPT_HANDLER 0x06, 0 ; 指令无效
INTERRUPT_HANDLER 0x07, 0 ; 设备不可用

INTERRUPT_HANDLER 0x08, 1 ; 双重错误
INTERRUPT_HANDLER 0x09, 0 ; 协处理器段超限
INTERRUPT_HANDLER 0x0A, 1 ; 无效任务状态段
INTERRUPT_HANDLER 0x0B, 1 ; 段无效

INTERRUPT_HANDLER 0x0C, 1 ; 斩断错误
INTERRUPT_HANDLER 0x0D, 1 ; 一般性保护异常
INTERRUPT_HANDLER 0x0E, 1 ; 缺页错误
INTERRUPT_HANDLER 0x0F, 0 ; 保留

INTERRUPT_HANDLER 0x10, 0 ; 浮点异常
INTERRUPT_HANDLER 0x11, 0 ; 对齐检测 
INTERRUPT_HANDLER 0x12, 0 ; 机器检测
INTERRUPT_HANDLER 0x13, 0 ; SIMD 浮点异常

INTERRUPT_HANDLER 0x14, 0 ; 虚拟化异常
INTERRUPT_HANDLER 0x15, 0 ; 控制保护异常
INTERRUPT_HANDLER 0x16, 0 ; 保留
INTERRUPT_HANDLER 0x17, 0 ; 保留

INTERRUPT_HANDLER 0x18, 0 ; 保留
INTERRUPT_HANDLER 0x19, 0 ; 保留
INTERRUPT_HANDLER 0x1A, 0 ; 保留
INTERRUPT_HANDLER 0x1B, 0 ; 保留

INTERRUPT_HANDLER 0x1C, 0 ; 保留
INTERRUPT_HANDLER 0x1D, 0 ; 保留
INTERRUPT_HANDLER 0x1E, 0 ; 保留
INTERRUPT_HANDLER 0x1F, 0 ; 保留

INTERRUPT_HANDLER 0x20, 0 ; 时钟中断
INTERRUPT_HANDLER 0x21, 0 ; 保留
INTERRUPT_HANDLER 0x22, 0 ; 保留
INTERRUPT_HANDLER 0x23, 0 ; 保留
INTERRUPT_HANDLER 0x24, 0 ; 保留
INTERRUPT_HANDLER 0x25, 0 ; 保留
INTERRUPT_HANDLER 0x26, 0 ; 保留
INTERRUPT_HANDLER 0x27, 0 ; 保留
INTERRUPT_HANDLER 0x28, 0 ; 保留
INTERRUPT_HANDLER 0x29, 0 ; 保留
INTERRUPT_HANDLER 0x2A, 0 ; 保留
INTERRUPT_HANDLER 0x2B, 0 ; 保留
INTERRUPT_HANDLER 0x2C, 0 ; 保留
INTERRUPT_HANDLER 0x2D, 0 ; 保留
INTERRUPT_HANDLER 0x2E, 0 ; 保留
INTERRUPT_HANDLER 0x2F, 0 ; 保留


section .data
; 上面宏定义的中断处理函数的起始地址表
; 一个地址占 double word (dd)
global handler_entry_table
handler_entry_table:
    dd interrupt_handler_0x00
    dd interrupt_handler_0x01
    dd interrupt_handler_0x02
    dd interrupt_handler_0x03
    dd interrupt_handler_0x04
    dd interrupt_handler_0x05
    dd interrupt_handler_0x06
    dd interrupt_handler_0x07
    dd interrupt_handler_0x08
    dd interrupt_handler_0x09
    dd interrupt_handler_0x0A
    dd interrupt_handler_0x0B
    dd interrupt_handler_0x0C
    dd interrupt_handler_0x0D
    dd interrupt_handler_0x0E
    dd interrupt_handler_0x0F
    dd interrupt_handler_0x10
    dd interrupt_handler_0x11
    dd interrupt_handler_0x12
    dd interrupt_handler_0x13
    dd interrupt_handler_0x14
    dd interrupt_handler_0x15
    dd interrupt_handler_0x16
    dd interrupt_handler_0x17
    dd interrupt_handler_0x18
    dd interrupt_handler_0x19
    dd interrupt_handler_0x1A
    dd interrupt_handler_0x1B
    dd interrupt_handler_0x1C
    dd interrupt_handler_0x1D
    dd interrupt_handler_0x1E
    dd interrupt_handler_0x1F

    dd interrupt_handler_0x20 ; 时钟中断
    dd interrupt_handler_0x21
    dd interrupt_handler_0x22
    dd interrupt_handler_0x23
    dd interrupt_handler_0x24
    dd interrupt_handler_0x25
    dd interrupt_handler_0x26
    dd interrupt_handler_0x27
    dd interrupt_handler_0x28
    dd interrupt_handler_0x29
    dd interrupt_handler_0x2A
    dd interrupt_handler_0x2B
    dd interrupt_handler_0x2C
    dd interrupt_handler_0x2D
    dd interrupt_handler_0x2E
    dd interrupt_handler_0x2F
