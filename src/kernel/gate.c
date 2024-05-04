#include <yui/interrupt.h>
#include <yui/assert.h>
#include <yui/debug.h>
#include <yui/syscall.h>
#include <yui/task.h>
#include <yui/console.h>
#include <yui/memory.h>
#include <yui/device.h>
#include <yui/string.h>
#include <yui/buffer.h>
#include <yui/fs.h>

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

// 系统调用-test
static u32 sys_test()
{
    DEBUG("sys_test called!!!\n");
    return 255;
}

extern time_t sys_time();
extern mode_t sys_umask();

extern int sys_mkdir();
extern int sys_rmdir();

extern int sys_link();
extern int sys_unlink();

extern fd_t sys_open();
extern fd_t sys_creat();
extern void sys_close();

extern int sys_read();
extern int sys_write();

extern int sys_lseek();

extern int sys_chdir();
extern int sys_chroot();
extern char *sys_getcwd();

extern int sys_readdir();

extern void console_clear();

extern int sys_stat();
extern int sys_fstat();

extern int sys_mknod();

extern int sys_mount();
extern int sys_umount();

extern int sys_mkfs();

extern int sys_brk();
extern int sys_mmap();
extern int sys_munmap();

extern void sys_execve();

void syscall_init()
{
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = sys_default;
    }

    syscall_table[SYS_NR_TEST] = sys_test;

    syscall_table[SYS_NR_EXIT] = task_exit;

    syscall_table[SYS_NR_FORK] = task_fork;

    syscall_table[SYS_NR_READ] = sys_read;

    syscall_table[SYS_NR_WAITPID] = task_waitpid;

    syscall_table[SYS_NR_LINK] = sys_link;

    syscall_table[SYS_NR_UNLINK] = sys_unlink;

    syscall_table[SYS_NR_TIME] = sys_time;

    syscall_table[SYS_NR_LSEEK] = sys_lseek;

    syscall_table[SYS_NR_WRITE] = sys_write;

    syscall_table[SYS_NR_UMASK] = sys_umask;

    syscall_table[SYS_NR_GETPID] = sys_getpid;

    syscall_table[SYS_NR_MKDIR] = sys_mkdir;
    syscall_table[SYS_NR_RMDIR] = sys_rmdir;

    syscall_table[SYS_NR_OPEN] = sys_open;
    syscall_table[SYS_NR_CREAT] = sys_creat;
    syscall_table[SYS_NR_CLOSE] = sys_close;

    syscall_table[SYS_NR_BRK] = sys_brk;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;

    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YEILD] = task_yield;

    syscall_table[SYS_NR_CHDIR] = sys_chdir;
    syscall_table[SYS_NR_CHROOT] = sys_chroot;
    syscall_table[SYS_NR_GETCWD] = sys_getcwd;

    syscall_table[SYS_NR_READDIR] = sys_readdir;
    syscall_table[SYS_NR_CLEAR] = console_clear;

    syscall_table[SYS_NR_STAT] = sys_stat;
    syscall_table[SYS_NR_FSTAT] = sys_fstat;

    syscall_table[SYS_NR_MKNOD] = sys_mknod;

    syscall_table[SYS_NR_MOUNT] = sys_mount;
    syscall_table[SYS_NR_UMOUNT] = sys_umount;

    syscall_table[SYS_NR_MKFS] = sys_mkfs;

    syscall_table[SYS_NR_MMAP] = sys_mmap;
    syscall_table[SYS_NR_MUNMAP] = sys_munmap;

    syscall_table[SYS_NR_EXECVE] = sys_execve;
}
