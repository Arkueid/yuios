; 1号扇区中的引导记录
; 1. 清屏，初始化寄存器，在实模式下打印字符
; 2. 读取硬盘第二个扇区开始的内核loader到内存0x1000位置
; 3. 检测loader是否正确加载（通过判断第前两个字节是否为55aa
[org 0x7c00]

; 设置屏幕模式为文本模式，清除屏幕
mov ax, 3
int 0x10

; 初始化段寄存器
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

mov si, booting
call print

mov edi, 0x1000 ; 读取的目标内存
mov ecx, 2 ; 起始扇区
mov bl, 4 ; 扇区数量

call read_disk ; 读取内核加载器

cmp word [0x1000], 0x55aa
jnz error

jmp 0:0x1002

; 阻塞
jmp $

; 读磁盘，需要与磁盘控制器的端口互传数据，有等待时间
; 为了简单实现，连续使用三个jmp $+2
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

booting:
    db "Booting...", 10, 13, 0 ; \n \r 字符串结束符

error:
    mov si, .msg
    call print
    hlt ; 让CPU停止
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0
    

times 510 - ($ - $$) db 0

; 主引导扇区的最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa ; 小端储存