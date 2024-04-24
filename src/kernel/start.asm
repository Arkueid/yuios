[bits 32]

magic   equ 0xe85250d6
i386    equ 0
length  equ header_end - header_start

section .multiboot2
header_start:
    dd magic 
    dd i386
    dd length
    dd -(magic + i386 + length)

    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:

extern console_init
extern memory_init
extern kernel_init
extern gdt_init

section .text

global _start
_start:
    push ebx ; ards count
    push eax ; magic
    ; call kernel_init

    call console_init   ; 初始化控制台
    call gdt_init       ; 初始化gdt
    call memory_init    ; 内存初始化
    call kernel_init    ; 内核初始化
    
    jmp $ ; 阻塞