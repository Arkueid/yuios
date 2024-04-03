; 内核loader
; 1. 检测内存 ards 内存范围描述符
; 2. 初始化gdt
; 3. 开启a20地址线，开启保护模式，启动内核
[org 0x1000]

dw 0x55aa ; 用于错误判断

mov si, loading
call print

detect_memory:
    xor ebx, ebx ; 清空ebx
    
    ; es:di 结构体的保存位置
    mov ax, 0
    mov es, ax ; 段寄存器不能直接mov立即数
    mov edi, ards_buffer

    mov edx, 0x534d4150 ; 固定签名

.next:
    ; 子功能号 - 遍历主机上所有内存
    mov eax, 0xe820 
    ; ards 结构大小
    mov ecx, 20
    ; 0x15 系统调用
    int 0x15

    ; 如果 cf 置位 表示出错
    jc error

    ; 指针指向下一个结构体
    add di, cx

    inc word [ards_count]

    cmp ebx, 0
    jnz .next

    mov si, detecting
    call print

    mov cx, [ards_count]
    mov si, 0

    jmp prepare_protected_mode

prepare_protected_mode:
    cli ; 关中断

    in al, 0x92 ; 开启A20地址线
    or al, 0b10
    out 0x92, al

    ; 加载gdt
    lgdt [gdt_ptr]

    ; 启动保护模式
    mov eax, cr0
    or eax, 1 ; 高位置1
    mov cr0, eax

    ; 跳转来刷新段寄存器
    jmp dword code_selector:protected_mode

; 阻塞
jmp $

print:
    mov ah, 0x0e
.next:
    mov al, [si]
    cmp al, 0
    jz .done
    int 0x10
    inc si
    jmp .next
.done:
    ret

loading:
    db "Loading Yui...", 10, 13, 0
detecting:
    db "Detecting Memory Success...", 10, 13, 0

error:
    mov si, .msg
    call print
    hlt ; 让CPU停止
    jmp $
    .msg db "Loading Error!!!", 10, 13, 0



[bits 32]
protected_mode:
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 内存栈从此处开始
    mov esp, 0x10000

    mov edi, 0x10000
    mov ecx, 10
    mov bl, 200

    call read_disk

    jmp dword code_selector:0x10000

    ud2 ; 表示出错
jmp $

read_disk:
    mov dx, 0x1f2
    mov al, bl
    out dx, al

    inc dx ; 0x1f3
    mov al, cl
    out dx, al

    inc dx ; 0x1f4
    shr ecx, 8
    mov al, cl
    out dx, al

    inc dx ; 0x1f5
    shr ecx, 8
    mov al, cl
    out dx, al

    inc dx ; 0x1f6
    shr ecx, 8
    and cl, 0b1111 ; 取低四位
    
    mov al, 0b1110_0000
    or al, cl
    out dx, al 

    inc dx ; 0x1f7
    mov al, 0x20
    out dx, al

    xor ecx, ecx ; 将ecx清空
    mov cl, bl ; 得到读写扇区的数量

    .read:
        push cx
        call .waits ; 等待扇区准备完毕
        call .reads ; 读取一个扇区
        pop cx
        loop .read
    ret

    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx
            jmp $+2
            jmp $+2
            jmp $+2
            and al, 0b1000_1000
            cmp al, 0b0000_1000
            jnz .check
        ret

    .reads:
        mov dx, 0x1f0
        mov cx, 256
        .readw:
            in ax, dx
            jmp $+2
            jmp $+2
            jmp $+2
            mov [edi], ax
            add edi, 2
            loop .readw
        ret

; 选择子 RPL(2位)-TI(1位)-Index(13位)
; 小端 且 高3位为0
code_selector equ (1 << 3) ; 一个描述符8个字节
data_selector equ (2 << 3)
; 内存开始位置 基地址
memory_base equ 0
; 内存界限
memory_limit equ ((1024 * 1024 * 1024 * 4) / (1024 * 4)) - 1

gdt_ptr:
    dw (gdt_end-gdt_base) - 1 ; 界限 长度-1
    dd gdt_base ; 开始位置

; 80386 gdt 结构 32 位
gdt_base:
    dd 0, 0 ; 0号描述符
gdt_code: ; 代码段
    dw memory_limit & 0xffff ; 段界限 低16位
    dw memory_base & 0xffff ; 段基址 低16位
    db (memory_base >> 16) & 0xff ; 段基址 +8位
    db 0b_1_00_1_1_0_1_0 ; 段属性
    ; 4k
    db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff ; 高8位
gdt_data: ; 数据段
    dw memory_limit & 0xffff ; 段界限 低16位
    dw memory_base & 0xffff ; 段基址 低16位
    db (memory_base >> 16) & 0xff ; 段基址 +8位
    db 0b_1_00_1_0_0_1_0 ; 段属性
    ; 4k
    db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff ; 高8位
gdt_end:

ards_count:
    dw 0
ards_buffer:
