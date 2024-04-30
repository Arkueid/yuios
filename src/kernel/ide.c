#include <yui/ide.h>
#include <yui/io.h>
#include <yui/printk.h>
#include <yui/stdio.h>
#include <yui/memory.h>
#include <yui/interrupt.h>
#include <yui/task.h>
#include <yui/string.h>
#include <yui/assert.h>
#include <yui/debug.h>

#define IDE_IOBASE_PRIMARY 0x1f0   // 主通道基址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基址

// IDE 寄存器偏移
#define IDE_DATA 0x0000     // 数据寄存器
#define IDE_ERR 0x0001      // 错误寄存器
#define IDE_FEATURE 0x0001  // 功能寄存器
#define IDE_SECTOR 0x0002   // 扇区数量
#define IDE_LBA_LOW 0x0003  // lba 低字节
#define IDE_LBA_MID 0x0004  // lba 中字节
#define IDE_LBA_HIGH 0x0005 // lba 高字节

#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xec // 识别命令

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA

ide_ctrl_t controllers[IDE_CTRL_NR];

void ide_handler(int vector)
{
    send_eoi(vector); // 向中断控制器发送中断处理结束信号

    // 得到中断向量对应的控制器
    // 区分 硬盘1中断还是2中断
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];

    u8 state = inb(ctrl->iobase + IDE_STATUS);
    DEBUG("harddisk interrupt vector %d state 0x%x\n",
          vector, state);

    if (ctrl->waiter)
    {
        // 如果有阻塞进程，则唤醒进程
        task_unblock(ctrl->waiter);
        ctrl->waiter = NULL;
    }
}

static u32 ide_error(ide_ctrl_t *ctrl)
{
    u8 error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        DEBUG("bad lock\n");
    if (error & IDE_ER_UNC)
        DEBUG("uncorrectable data\n");
    if (error & IDE_ER_MC)
        DEBUG("media change\n");
    if (error & IDE_ER_IDNF)
        DEBUG("id not found\n");
    if (error & IDE_ER_MCR)
        DEBUG("media change requested\n");
    if (error & IDE_ER_ABRT)
        DEBUG("abort\n");
    if (error & IDE_ER_TK0NF)
        DEBUG("track 0 not found\n");
    if (error & IDE_ER_AMNF)
        DEBUG("address mark not found\n");
}

static u32 ide_busy_wait(ide_ctrl_t *ctrl, u8 mask)
{
    while (true)
    {
        u8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 出错
        {
            ide_error(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((state & mask) == mask) // 等待结束
            return 0;
    }
}

// 选择磁盘
static void ide_select_drive(ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

// 选择扇区
static void ide_select_sector(ide_disk_t *disk, u32 lba, u8 count)
{
    // 输出功能，可省略
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);

    // lba 低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);

    // lba 中字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, (lba >> 8) & 0xff);

    // lba 高字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, (lba >> 16) & 0xff);

    // lba 最高四位 + 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);

    disk->ctrl->active = disk;
}

static void ide_pio_read_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

static void ide_pio_write_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

// PIO 方式读取磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, index_t lba)
{
    assert(count > 0);

    assert(!get_interrupt_state()); // 异步方式读写，不允许中断

    ide_ctrl_t *ctrl = disk->ctrl;

    lock_accquire(&ctrl->lock);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        // 阻塞当前进程，直到磁盘完成读取并发送中断信号
        task_t *task = running_task();
        if (task->state == TASK_RUNNING)
        {
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }

        ide_busy_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *)offset);
    }

    lock_release(&ctrl->lock);
    return 0;
}

// pio 方式写磁盘
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, index_t lba)
{
    assert(count > 0);
    assert(!get_interrupt_state()); // 异步方式读写，不允许中断

    ide_ctrl_t *ctrl = disk->ctrl;

    lock_accquire(&ctrl->lock);

    DEBUG("write lba 0x%x\n", lba);

    ide_select_drive(disk);

    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (u16 *)offset);

        task_t *task = running_task();
        // TODO: 系统初始化时不能使用异步方式
        if (task->state == TASK_RUNNING) 
        {
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }

        ide_busy_wait(ctrl, IDE_SR_NULL);
    }

    lock_release(&ctrl->lock);
    return 0;
}

static void ide_ctrl_init()
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;

        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }

        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);

            disk->ctrl = ctrl;

            if (didx)
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
        }
    }
}

void ide_init()
{
    DEBUG("IDE INIT...\n");

    ide_ctrl_init();
    // 注册硬盘中断，并打开中断
    set_interrupt_handler(IRQ_HARDDISK, ide_handler);
    set_interrupt_handler(IRQ_HARDDISK2, ide_handler);
    set_interrupt_mask(IRQ_HARDDISK, true);
    set_interrupt_mask(IRQ_HARDDISK2, true);
    set_interrupt_mask(IRQ_CASCADE, true);
}