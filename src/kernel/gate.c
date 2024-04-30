#include <yui/interrupt.h>
#include <yui/assert.h>
#include <yui/debug.h>
#include <yui/syscall.h>
#include <yui/task.h>
#include <yui/console.h>
#include <yui/memory.h>

#define SYSCALL_SIZE 256

handler_t syscall_table[SYSCALL_SIZE];

extern void task_yield();

// 检查系统调用号是否正确
void syscall_check(u32 nr)
{
    if (nr >= SYSCALL_SIZE)
    {
        panic("syscall nr error!!!");
    }
}

// 默认系统调用函数
static void sys_default()
{
    panic("syscall not implemented!!!");
}

#include <yui/string.h>
#include <yui/ide.h>

extern ide_ctrl_t controllers[2];

// 系统调用-test
static u32 sys_test()
{
    u16 *buf = (u16 *)alloc_kpage(1);
    DEBUG("pio read buffer 0x%p\n", buf);
    ide_disk_t *disk = &controllers[0].disks[0];
    ide_pio_read(disk, buf, 4, 0);
    BMB;

    memset(buf, 0x5a, 512);

    ide_pio_write(disk, buf, 1, 1);

    free_kpage((u32)buf, 1);

    return 255;
}


int32 sys_write(fd_t fd, char *buf, u32 len)
{
    if (fd == stdout || fd == stderr)
    {
        return console_write(buf, len);
    }
    // TODO: write
    panic("write error: unknown fd %d!!!", fd);
    return 0;
}

extern time_t sys_time();

void syscall_init()
{
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = sys_default;
    }

    syscall_table[SYS_NR_TEST] = sys_test;

    syscall_table[SYS_NR_EXIT] = task_exit;

    syscall_table[SYS_NR_FORK] = task_fork;

    syscall_table[SYS_NR_WAITPID] = task_waitpid;

    syscall_table[SYS_NR_TIME] = sys_time;

    syscall_table[SYS_NR_WRITE] = sys_write;

    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_BRK] = sys_brk;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;
    
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YEILD] = task_yield;
}

