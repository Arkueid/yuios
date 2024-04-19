#include <yui/io.h>
#include <yui/interrupt.h>
#include <yui/assert.h>
#include <yui/debug.h>

#define PIT_CHAN0_REG 0x40
#define PIT_CHAN2_REG 0x42
#define PIT_CTRL_REG 0x43

#define HZ 100
#define OSCILLATOR 1193182 // 震荡器频率
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

void clock_handler(int vector)
{
    assert(vector == 0x20);
    send_eoi(vector);

    jiffies++;
    DEBUG("clock jiffies %d ...\n", jiffies);
}

// 初始化可编程计数器
void pit_init()
{
    // 00 0号计数器
    // 11 先读写低字节，再读写高字节
    // x10 模式2
    // 0 表示二进制计数
    // 递减计数
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);
}

// 初始化时钟
void clock_init()
{
    pit_init();
    set_interrupt_handler(IRQ_CLOCK, clock_handler);
    set_interrupt_mask(IRQ_CLOCK, true);
}