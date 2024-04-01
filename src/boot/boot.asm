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

; 注册中断函数
mov word [0x0 * 4], interrupt ; 偏移量
mov word [0x0 * 4 + 2], 0 ; 段地址

; 0x80 即中断向量表的索引
mov dx, 0
mov ax, 1
mov bx, 0

xchg bx, bx
div bx  ; dx:ax / bx

; 阻塞
jmp $

interrupt:
    mov si, booting_string
    call print
    iret


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

booting_string:
    db "Booting...", 10, 13, 0 ; \n \r 字符串结束符

times 510 - ($ - $$) db 0

; 主引导扇区的最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa ; 小端储存