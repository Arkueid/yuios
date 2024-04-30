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

extern device_init
extern console_init
extern memory_init
extern kernel_init
extern gdt_init
extern gdt_ptr

code_selector equ (1 << 3)
data_selector equ (2 << 3)

section .text

global _start
_start:
    push ebx ; ards count
    push eax ; magic

    call device_init    ; 初始化设备
    call console_init   ; 初始化控制台
    call gdt_init       ; 初始化gdt

    lgdt [gdt_ptr]

    jmp dword code_selector:_next

_next:

    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call memory_init    ; 内存初始化

    mov esp, 0x10000

    call kernel_init    ; 内核初始化
    
    jmp $ ; 阻塞