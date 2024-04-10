#include <yui/console.h>
#include <yui/io.h>
#include <yui/string.h>

#define CRT_ADDR_REG 0x3D4 // 地址寄存器
#define CRT_DATA_REG 0x3D5 // 数据寄存器

#define CRT_START_ADDR_H 0xC // 显示内存起始位置 高位
#define CRT_START_ADDR_L 0xD // 显示内存起始位置 低位
#define CRT_CURSOR_H 0xE     // 光标位置 高位
#define CRT_CURSOR_L 0xF     // 光标位置 低位

// 屏幕坐标
#define MEM_BASE 0xB8000              // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕总字节数

// 控制字符
#define ASCII_NULL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07 // \a
#define ASCII_BS 0x08  // \b
#define ASCII_HT 0x09  // \t
#define ASCII_LF 0x0A  // \n
#define ASCII_VT 0x0B  // \v
#define ASCII_FF 0x0C  // \f
#define ASCII_CR 0x0D  // \r
#define ASCII_DEL 0x7F

// static 全局声明，只能在定义的文件中可见
static u32 screen; // 记录当前显示 开始的起始位置

static u32 pos; // 光标当前显存位置

static u32 x, y; // 光标 当前在80*35中的位置

static u8 attr = 7;        // 字符样式
static u16 erase = 0x0720; // 空格

// 获取当前显示的内存开始位置
static void get_screen()
{
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    screen = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    screen |= inb(CRT_DATA_REG);
    screen <<= 1;       // 两个字节
    screen += MEM_BASE; // 内存中的位置
}

// 设置屏幕显示开始的位置
static void set_screen()
{
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

// 获取光标位置
static void get_cursor()
{
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    pos = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    pos |= inb(CRT_DATA_REG);
    pos <<= 1;       // 两个字节
    pos += MEM_BASE; // 内存中的位置

    get_screen();

    u32 delta = (pos - screen) >> 1;
    x = delta % WIDTH;
    y = delta / WIDTH;
}

// 设置光标位置
static void set_cursor()
{
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    // 1字节
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 1) & 0xff);
}

// 清空屏幕
void console_clear()
{
    screen = MEM_BASE;
    pos = MEM_BASE;
    x = y = 0;
    set_screen();
    set_cursor();

    u16 *ptr = (u16 *)MEM_BASE;
    while (ptr < (u16 *)MEM_END)
    {
        *ptr++ = erase;
    }
}

// 退格 \b
static void command_bs()
{
    if (x)
    {
        x--;
        pos -= 2;
        *(u16 *)pos = erase;
    }
}

// 水平制表符\t
static void command_ht()
{
}

// 向上滚一行
static void scroll_up()
{
    // 显存中还能往下写一行
    if (screen + SCR_SIZE + ROW_SIZE < MEM_END)
    {

        // 将当前屏幕复制到显存开始处
        memcpy((void *)MEM_BASE, (void *)screen, SCR_SIZE);
        // 光标移动到新屏幕对应位置
        pos = pos - screen + MEM_BASE;
        screen = MEM_BASE;
    }
    // 清空下一行
    u32 *ptr = (u32 *)(screen + SCR_SIZE);
    for (size_t i = 0; i < WIDTH; i++)
    {
        *ptr++ = erase;
    }

    // 屏幕往上滚动一行
    screen += ROW_SIZE;
    // 光标向下移动
    pos += ROW_SIZE;
    set_screen();
}

// 换行\n
static void command_lf()
{
    if (y + 1 < HEIGHT)
    {
        y++;
        pos += ROW_SIZE;
        return;
    }
    scroll_up(); // 屏幕满了往上滚一行
}

static void command_vt()
{
}

static void command_ff()
{
}

// 回车\r
static void command_cr()
{
    pos -= (x << 1);
    x = 0;
}

// 删除当前字符
static void command_del()
{
    *(u16 *)pos = erase;
}

void console_write(char *buf, u32 count)
{
    char ch;
    while (count--)
    {
        ch = *buf++;
        switch (ch)
        {
        case ASCII_NULL:
            break;
        case ASCII_ENQ:
            break;
        case ASCII_BEL:
            // \todo \a
            break;
        case ASCII_BS: // \b
            command_bs();
            break;
        case ASCII_HT: //
            command_bs();
            break;
        case ASCII_LF:
            command_lf();
            command_cr();
            break;
        case ASCII_VT:
            break;
        case ASCII_FF:
            command_ff();
            break;
        case ASCII_CR:
            command_cr();
            break;
        case ASCII_DEL:
            command_del();
            break;
        default:
            if (x >= WIDTH)
            {
                x -= WIDTH;
                pos -= ROW_SIZE;
                command_lf();
                command_cr();
            }
            *(char *)pos++ = ch;
            *(char *)pos++ = attr;
            x++;
            break;
        }
        set_cursor();
    }
}

void console_init()
{
    // get_screen();
    // screen = 80 * 2 + MEM_BASE; // 第二行
    // set_screen();
    // pos = (160 + 20) * 2 + MEM_BASE;
    // set_cursor();

    console_clear();
}