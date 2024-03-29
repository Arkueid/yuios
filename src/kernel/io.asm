[bits 32]

section .text

global inb ; u8 inb(u16 port);
inb:
    push ebp
    mov ebp, esp ; 保存栈帧

    xor eax, eax ; 清空 eax
    mov edx, [ebp + 8] ; port
    in al, dx ; 将端口号的低8位输入ax

    jmp $+2
    jmp $+2
    jmp $+2 ; 实现延迟，等待io响应


    leave ; 恢复栈帧
    ret

global inw ; u16 inw(u16 port);
inw:
    push ebp
    mov ebp, esp ; 保存栈帧

    xor eax, eax ; 清空 eax
    mov edx, [ebp + 8] ; port
    in ax, dx ; 将端口号的低8位输入ax

    jmp $+2
    jmp $+2
    jmp $+2 ; 实现延迟，等待io响应

    leave ; 恢复栈帧
    ret


global outb ; void outb(u16 port, u8 value);
outb:
    push ebp
    mov ebp, esp ; 保存栈帧

    xor eax, eax ; 清空 eax
    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; value
    out dx, al ; 将 al 中的 8bit 输出到dx

    jmp $+2
    jmp $+2
    jmp $+2 ; 实现延迟，等待io响应

    leave ; 恢复栈帧
    ret

global outw ; void outw(u16 port, u16 value);
outw: 
    push ebp
    mov ebp, esp ; 保存栈帧

    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; value
    out dx, ax

    jmp $+2
    jmp $+2
    jmp $+2

    leave
    ret