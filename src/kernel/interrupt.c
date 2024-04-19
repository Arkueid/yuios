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

static char *messages[] = {
    "#DE\0",
    "#DB\0",
    "#NMI\0",
    "#BP\0",
    "#OF\0",
    "#BR\0",
    "#UD\0",
    "#NM\0",
    "#DF\0",
    "#CSO\0",
    "#TS\0",
    "#NP\0",
    "#SS\0",
    "#GP\0",
    "#PF\0",
    "#Reserved\0",
    "#MF\0",
    "#AC\0",
    "#MC\0",
    "#XM/#XF\0",
    "#VE\0",
    "#CP\0",
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
        irq -= 8;  // 从片起始位置
    }

    if (enable)
    {
        outb(port, inb(port) & ~(1 << irq));
    }
    else
    {
        outb(port, inb(port) | (1 << irq));  // 关闭对应位的中断
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
    u32 vector0, u32 error, // 压入错误码和中断向量
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

extern void schedule();

void default_handler(int vector)
{
    // 向中断控制器发送中断结束信息
    send_eoi(vector);
    schedule();
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

    // @todo  中断屏蔽字 开键盘中断
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

    for (int i = 0x20; i < ENTRY_SIZE; i++)
    {
        handler_table[i] = default_handler;
    }

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;

    asm volatile("lidt idt_ptr\n");
}

void interrupt_init()
{
    pic_init();

    idt_init();
}