[bits 32]
; 中断程序入口

section .text

extern printk

global interrupt_handler
interrupt_handler:
    xchg bx, bx

    push message
    call printk
    add esp, 4 ; message 弹出，不需要 pop 返回值
    
    xchg bx, bx

    iret

section .data

message:
    db "default interrupt", 10, 0