[bits 32]

extern kernel_init

global _start
_start:
    ; mov byte [0xb8000], 'K'
    call kernel_init
    
    mov bx, 0
    div bx
    
    jmp $ ; 阻塞