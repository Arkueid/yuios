#include <yui/yui.h>
#include <yui/types.h>
#include <yui/io.h>
#include <yui/string.h>

// - CRT 地址寄存器     0x3D4
// - CRT 数据寄存器     0x3D5
// - CRT 光标位置 高位  0xE
// - CRT 光标位置 低位  0xF

// #define CRT_ADDR_REG 0x3d4
// #define CRT_DATA_REG 0x3d5

// #define CRT_CURSOR_H 0xe
// #define CRT_CURSOR_L 0xf

char message[] = "Hello, world!!!";
char buf[1024];

void kernel_init()
{

    // outb(CRT_ADDR_REG, CRT_CURSOR_H);// 输入 光标低位 地址

    // u16 pos = inb(CRT_DATA_REG) << 8; // 读取数据

    // outb(CRT_ADDR_REG, CRT_CURSOR_L);// 输入 光标高位 地址

    // pos |= inb(CRT_DATA_REG);// 相与合并高低位

    // outb(CRT_ADDR_REG, CRT_CURSOR_H);// 修改光标位置
    // outb(CRT_DATA_REG, 1);

    // outb(CRT_ADDR_REG, CRT_CURSOR_L);// 修改光标位置
    // outb(CRT_DATA_REG, 0b1101000);
    int res;
    res = strcmp(buf, message);
    strcpy(buf, message);
    res = strcmp(buf, message);
    strcat(buf, message);
    res = strcmp(buf, message);
    res = strlen(buf);
    res = sizeof(message);

    char *ptr = strchr(message, '!');
    ptr = strrchr(message, '!');

    memset(buf, 0, sizeof(buf));
    res = memcmp(buf, message, sizeof(message));
    memcpy(buf, message, sizeof(message));
    res = memcmp(buf, message, sizeof(message));
    ptr = memchr(buf, '!', sizeof(message));

    return;
}
