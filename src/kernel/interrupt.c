#include <yui/interrupt.h>
#include <yui/global.h>
#include <yui/debug.h>
#include <yui/printk.h>
#include <yui/assert.h>
#include <yui/io.h>
#include <yui/stdlib.h>

#define LOGK(fmt, args...) DEBUG(fmt, ##args)
// #define LOGK(fmt, args...)

#define ENTRY_SIZE 0x30 // 定义48个中断处理函数入口

// 中断控制器的端口
#define PIC_M_CTRL 0x20 // 主板的控制端口
#define PIC_M_DATA 0x21 // 主板的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束

gate_t idt[IDT_SIZE];
pointer_t idt_ptr;

handler_t handler_table[IDT_SIZE];

// 中断函数入口地址
extern handler_t handler_entry_table[ENTRY_SIZE];
extern void syscall_handler();
extern void page_fault();

static char *messages[] = {
    "Divide Error - #DE\0",
    "Debug Exception - #DB\0",
    "Non-maskable Interrupt - #NMI\0",
    "Breakpoint - #BP\0",
    "Overflow - #OF\0",
    "BOUND Range Exceeded - #BR\0",
    "Invalid Opcode - #UD\0",
    "Device Not Available - #NM\0",
    "Double Fault - #DF\0",
    "Coprocessor Segment Overrun - #CSO\0",
    "Invalid TSS - #TS\0",
    "Segment Not Present - #NP\0",
    "Stack-Segment Fault - #SS\0",
    "General Protection Fault - #GP\0",
    "Page Fault - #PF\0",
    "Reserved\0",
    "x87 FPU Floating-Point Error (Math Fault) - #MF\0",
    "Alignment Check - #AC\0",
    "Machine Check - #MC\0",
    "SIMD Floating-Point Exception - #XM/#XF\0",
    "Virtualization Exception - #VE\0",
    "Control Protection Exception - #CP\0",
};

// 通知中断控制器，中断处理结束
void send_eoi(int vector)
{
    if (vector >= 0x20 && vector < 0x28)
    {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30)
    {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

// 设置中断处理函数
void set_interrupt_handler(u32 irq, handler_t handler)
{
    assert(irq >= 0 && irq < 16);
    handler_table[IRQ_MASTER_NR + irq] = handler;
}

// 设置对应是否开启
void set_interrupt_mask(u32 irq, bool enable)
{
    assert(irq >= 0 && irq < 16);
    u16 port;
    if (irq < 8)
    {
        port = PIC_M_DATA;
    }
    else
    {
        port = PIC_S_DATA;
        irq -= 8; // 从片起始位置
    }

    if (enable)
    {
        outb(port, inb(port) & ~(1 << irq));
    }
    else
    {
        outb(port, inb(port) | (1 << irq)); // 关闭对应位的中断
    }
}

// 异常处理函数
void exception_handler(
    // 中断向量
    int vector,
    // 保存8个通用寄存器
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    // 栈上文
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error,     // 压入错误码和中断向量
    u32 eip, u32 cs, u32 eflags // 中断处理会先压入 eflags 和 ip
)
{
    char *message = NULL;
    if (vector < 22)
    {
        message = messages[vector];
    }
    else
    {
        message = messages[15];
    }

    printk("\nException: [0x%02x] %s \n", vector, message);
    printk("    VECTOR: 0x%02X\n", vector);
    printk("    ERROR:  0x%02X\n", error);
    printk("    EFLAGS: 0x%02X\n", eflags);
    printk("    CS:     0x%02X\n", cs);
    printk("    EIP:    0x%02X\n", eip);
    printk("    ESP:    0x%02X\n", esp);

    // 阻塞
    hang();
}

// 关中断，并返回之前 IF 位的值
bool interrupt_disable()
{
    asm volatile(
        "pushfl\n"        // 将当前eflags圧入栈中
        "cli\n"           // 清除IF位，此时外中断已经被屏蔽
        "popl %eax\n"     // 将刚才圧入的 eflags 弹出到eax
        "shrl $9, %eax\n" // 得到 eflags的第10位 if 位
        "andl $1, %eax\n" // 第10位以后的截断丢弃
    );
}

// 获得 IF 位的值
bool get_interrupt_state()
{
    asm volatile(
        "pushfl\n"        // 圧入eflags
        "popl %eax\n"     // 获取eflags的值
        "shrl $9, %eax\n" // 获取第10位
        "andl $1, %eax\n" // 丢弃其它位
    );
}

// 设置 IF 位
void set_interrupt_state(bool state)
{
    if (state)
        asm volatile("sti\n");
    else
        asm volatile("cli\n");
}

void default_handler(int vector)
{
    // 向中断控制器发送中断结束信息
    send_eoi(vector);
}

// 初始化中断控制器
void pic_init()
{
    outb(PIC_M_CTRL, 0b00010001); // ICW1: 边沿触发
    outb(PIC_M_DATA, 0x20);       // ICW2 起始端口号 0x20
    outb(PIC_M_DATA, 0b00000100); // ICW3 IR2从片
    outb(PIC_M_DATA, 0b00000001); // ICW4 8086模式

    outb(PIC_S_CTRL, 0b00010001); // ICW1: 边沿触发，级联 8295，需要ICW4
    outb(PIC_S_DATA, 0x28);       // ICW2 起始端口号 0x28
    outb(PIC_S_DATA, 2);          // ICW3 设置从片连接到主片的 IR2 引脚（第三个引脚）
    outb(PIC_S_DATA, 0b00000001); // ICW4 8086模式 正常EOI

    outb(PIC_M_DATA, 0b11111111); // 关闭所有中断
    outb(PIC_S_DATA, 0b11111111); // 关闭所有中断
}

void idt_init()
{
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        gate_t *gate = &idt[i];
        handler_t handler = handler_entry_table[i];

        gate->offset0 = (u32)handler & 0xffff;
        gate->offset1 = ((u32)handler >> 16) & 0xffff;
        gate->selector = 1 << 3; // 代码段
        gate->reserved = 0;
        gate->type = 0b1110; // 中断门
        gate->segment = 0;   // 系统段
        gate->DPL = 0;       // 内核态
        gate->present = 1;   // 有效
    }

    for (int i = 0; i < 0x20; i++)
    {
        handler_table[i] = exception_handler;
    }

    handler_table[0xe] = page_fault;

    for (int i = 0x20; i < ENTRY_SIZE; i++)
    {
        handler_table[i] = default_handler;
    }

    gate_t *gate = &idt[0x80];
    gate->offset0 = (u32)syscall_handler & 0xffff;
    gate->offset1 = ((u32)syscall_handler >> 16) & 0xffff;
    gate->selector = 1 << 3; // 代码段
    gate->reserved = 0;      // 保留不用
    gate->type = 0b1110;     // 中断门
    gate->segment = 0;       // 系统段
    gate->DPL = 3;           // 用户态
    gate->present = 1;       // 有效

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;

    asm volatile("lidt idt_ptr\n");
}

void interrupt_init()
{
    pic_init();

    idt_init();
}