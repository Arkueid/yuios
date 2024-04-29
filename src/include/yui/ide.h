#ifndef YUI_IDE_H
#define YUI_IDE_H

#include <yui/types.h>
#include <yui/mutex.h>

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器个数
#define IDE_DISK_NR 2 // 每个控制器可控制磁盘数量

// IDE 磁盘
typedef struct ide_disk_t
{
    char name[8];            // 磁盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;             // 磁盘选择
    bool master;             // 主盘
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];                  // 控制器名称
    lock_t lock;                   // 控制器锁
    u16 iobase;                    // io 寄存器基址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *active;            // 当前选择磁盘
} ide_ctrl_t;

/// @brief 将磁盘count个内容读入buf
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, index_t lba);
/// @brief 将buf的内容写入磁盘
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, index_t lba);

#endif
