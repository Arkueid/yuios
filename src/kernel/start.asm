[bits 32]

extern console_init
extern memory_init
extern kernel_init


global _start
_start:
    push ebx ; ards count
    push eax ; magic
    ; call kernel_init

    call console_init
    call memory_init
    
    jmp $ ; 阻塞