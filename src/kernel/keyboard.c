#include <yui/interrupt.h>
#include <yui/io.h>
#include <yui/assert.h>
#include <yui/debug.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CTRL_PORT 0x64

void keyboard_handler(int vector)
{
    assert(vector == 0x21);

    send_eoi(vector);

    u16 scancode = inb(KEYBOARD_DATA_PORT);
    DEBUG("keyboard input %d\n", scancode);
}

void keyboard_init()
{
    set_interrupt_handler(IRQ_KEYBOARD, keyboard_handler);
    set_interrupt_mask(IRQ_KEYBOARD, true);
}