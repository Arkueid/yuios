[bits 32]

extern console_init
extern memory_init
extern kernel_init
extern gdt_init


global _start
_start:
    push ebx ; ards count
    push eax ; magic
    ; call kernel_init

    call console_init   ; 初始化控制台
    call gdt_init       ; 初始化gdt
    call memory_init    ; 内存初始化
    call kernel_init    ; 内核初始化

    mov eax, 0 ; 系统调用号
    int 0x80
    
    jmp $ ; 阻塞