#ifndef YUI_FS_H
#define YUI_FS_H

#include <yui/types.h>
#include <yui/list.h>

#define BLOCK_SIZE 1024 // 块大小
#define SECTOR_SIZE 512 // 扇区大小

#define MINIX1_MAGIC 0x137f // 文件系统魔数
#define NAME_LEN 14         // 文件名长度

#define IMAP_NR 8 // indoe 位图块，最大值
#define ZMAP_NR 8 // 块位图，最大值

#define BLOCK_BITS (BLOCK_SIZE * 8) // 块位图大小
#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t)) // 块 inode 数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t)) // 块 dentry 数量
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(u16)) // 块索引数量
#define DIRECT_BLOCK (7) // 直接块数量
#define INDIRECT1_BLOCK BLOCK_INDEXES  // 一级间接块数量
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)  // 二级间接块数量
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK) // 全部块数量

#define SEPARATOR1 '/'
#define SEPARATOR2 '\\'
#define IS_SEPARATOR(c) (c == SEPARATOR1 || c == SEPARATOR2) 

typedef struct inode_desc_t
{
    u16 mode;    // 文件类型和属性(rwx 位)
    u16 uid;     // 用户id （文件拥有者标识）
    u32 size;    // 文件大小，字节数
    u32 mtime;   // 修改时间戳 UTC
    u8 gid;      // 组id，文件所属的组
    u8 nlinks;   // 链接数
    u16 zone[9]; // 直接0-6，间接7，双重间接8 逻辑块号
} inode_desc_t;

typedef struct inode_t
{
    inode_desc_t *desc;   // 描述符
    struct buffer_t *buf; // inode 描述符对应 buffer
    dev_t dev;            // 设备号
    index_t nr;           // i 节点号
    u32 count;            // 引用计数
    time_t atime;         // 访问时间
    time_t ctime;         // 创建时间
    list_node_t node;     // 链表节点
    dev_t mount;          // 挂载设备
} inode_t;

typedef struct super_desc_t
{
    u16 inodes;        // 节点数
    u16 zones;         // 逻辑块数
    u16 imap_blocks;   // i节点位图所占用的数据块数
    u16 zmap_blocks;   // 逻辑块位图所占用的数据块数
    u16 firstdatazone; // 第一个数据逻辑块号
    u16 log_zone_size; // log2 每逻辑块数据块数
    u32 max_size;      // 文件最大长度
    u16 magic;         // 文件系统魔数
} super_desc_t;

typedef struct super_block_t
{
    super_desc_t *desc;              // 超级块秒数符
    struct buffer_t *buf;            // 超级块描述符 缓冲
    struct buffer_t *imaps[IMAP_NR]; // inode 位图缓冲
    struct buffer_t *zmaps[ZMAP_NR]; // 块位图缓冲
    dev_t dev;                       // 设备号
    list_t inode_list;               // 使用中 inode 链表
    inode_t *iroot;                  // 根目录 inode
    inode_t *imount;                 // 挂载到的 inode
} super_block_t;

// 文件目录项结构
typedef struct dentry_t
{
    u16 nr;              // i 节点
    char name[NAME_LEN]; // 文件名
} dentry_t;

super_block_t *get_super(dev_t dev); // 获得 dev 对应的超级块

super_block_t *read_super(dev_t dev); // 读取 dev 对应的超级块

index_t balloc(dev_t dev);          // 分配一个文件块

void bfree(dev_t dev, index_t idx); // 释放一个文件块

index_t ialloc(dev_t dev);          // 分配一个inode

void ifree(dev_t dev, index_t idx); // 释放一个inode

index_t bmap(inode_t *inode, index_t block, bool create);

inode_t *get_root_inode(); // 获取根目录 inode
inode_t *iget(dev_t dev, index_t nr); // 获得设备 dev 的 nr inode

void iput(inode_t *inode);  // 释放 inode

#endif