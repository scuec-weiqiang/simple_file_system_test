/**
 * @FilePath: /simple_file_system_test/ext2.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:54
 * @LastEditTime: 2025-06-03 23:50:39
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "stdint.h"
#include "bitmap.h"
#include "malloc.h"
#include "virtdisk.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"

#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_INODE_DENSITY_PER_GIB 2048

typedef struct ext2_super_block {
    uint64_t magic;
    uint64_t inodes_count;      // inode总数
    uint64_t free_inodes_count; // 空闲inode数
    uint64_t blocks_count;      // 块总数
    uint64_t free_blocks_count; // 空闲块数
    uint64_t block_size;    // 块大小(字节)
    // ... 其他字段
}ext2_super_block_t;

typedef struct ext2_group_descriptor {
    uint64_t block_bitmap_start_idx;     // 块位图起始索引
    uint64_t inode_bitmap_start_idx;     // inode位图起始索引
    uint64_t inode_table_start_idx;    // inode表起始索引
    uint64_t data_block_start_idx;  // 第一个数据块起始索引
}ext2_group_descriptor_t;

typedef struct ext2_inode {
    uint32_t type;        // 文件类型
    uint32_t priv;        // 权限
    uint64_t size;        // 文件大小(字节)
    uint64_t ctime;       // 创建时间
    uint64_t blk_idx[13];  // 所在块号
}ext2_inode_t;


typedef struct ext2_fs
{
    ext2_super_block_t *super; 
    ext2_group_descriptor_t *group; 
    bitmap_t *block_bitmap; 
    bitmap_t *inode_bitmap; 
    ext2_inode_t *inode_table; 
}ext2_fs_t;

uint64_t super_block_pos_start = 0,super_block_num = 0;
uint64_t group_block_pos_start = 0,group_block_num = 0;
uint64_t block_bitmap_block_pos_start = 0,block_bitmap_block_num = 0;
uint64_t inode_bitmap_block_pos_start = 0,inode_bitmap_block_num = 0;
uint64_t inode_table_block_pos_start = 0,inode_table_block_num = 0;
uint64_t data_block_pos_start = 0,data_block_num;


/**
 * @brief 创建一个ext2文件系统
 *
 * 该函数用于创建一个新的ext2文件系统。它会为文件系统分配内存，并初始化其超级块、组描述符、块位图、inode位图以及inode表。
 *
 * @return 指向新创建的ext2文件系统的指针，如果创建失败则返回NULL。
 */
ext2_fs_t* ext2_fs_create()
{
    // 分配 ext2_fs_t 结构体内存
    ext2_fs_t* fs = malloc(sizeof(ext2_fs_t));
    if(fs==NULL)
    {
        printf("err0\n");
        return NULL;
    }

    // 分配 ext2_super_block_t 结构体内存，并初始化
    fs->super = (ext2_super_block_t *)malloc(BLOCK_SIZE);
    // 设置文件系统魔数
    fs->super->magic = EXT2_SUPER_MAGIC; 
    // 设置 inode 数量
    fs->super->inodes_count = 128;
    // 设置块大小
    fs->super->block_size = BLOCK_SIZE;
    // 设置块数量
    fs->super->blocks_count = DISK_SIZE / fs->super->block_size;

    // 分配 ext2_group_descriptor_t 结构体内存
    fs->group = (ext2_group_descriptor_t *)malloc(BLOCK_SIZE);
    // 创建块位图
    fs->block_bitmap = bitmap_create(fs->super->blocks_count);  
    // 创建 inode 位图
    fs->inode_bitmap = bitmap_create(fs->super->inodes_count);
    // 分配 inode 表内存
    fs->inode_table = (ext2_inode_t *)malloc(sizeof(ext2_inode_t) * fs->super->inodes_count);

    // 打印创建成功信息
    printf("create!\n");
    return fs;
}

/**
 * @brief 格式化 ext2 文件系统
 *
 * 该函数用于初始化并格式化 ext2 文件系统。它将文件系统结构（如超级块、组描述符、位图和 inode 表）写入磁盘，
 * 并配置各个结构的位置和大小。
 *
 * @param fs ext2 文件系统结构体指针
 *
 * @return 格式化成功返回 0，失败返回 -1
 */
int64_t ext2_fs_format(ext2_fs_t *fs)
{
    uint64_t now_block_pos = 0;

    // 配置super_block
    super_block_pos_start = now_block_pos;
    super_block_num = 1;
    if(fs->super == NULL)
    {
        return -1;
    }
    fs->super->magic = EXT2_SUPER_MAGIC; 
    fs->super->free_blocks_count = DISK_SIZE / fs->super->block_size;
    fs->super->free_inodes_count = 128;
    now_block_pos += super_block_num;


    // 配置group_descriptor
    group_block_pos_start = now_block_pos; //记录group的位置
    super_block_num = 1;
    //不急着配置group_descriptor,先预留空间,因为他的值要和后面的位图和inode表的位置有关
    
    if(fs->group == NULL)
    {
        return -1;
    }
    now_block_pos += group_block_num;


    // 配置两个位图
    block_bitmap_block_pos_start = now_block_pos;
    if(fs->block_bitmap == NULL)
    {
        return -1;
    }
    block_bitmap_block_num = (bitmap_get_bytes_num(fs->block_bitmap)+511)/BLOCK_SIZE; //计算块位图所占的块数
    now_block_pos += block_bitmap_block_num;

    inode_bitmap_block_pos_start = now_block_pos;
    if(fs->inode_bitmap == NULL)
    {
        return -1;
    }
    inode_bitmap_block_num = (bitmap_get_bytes_num(fs->inode_bitmap)+511)/BLOCK_SIZE; //计算inode位图所占的块数
    now_block_pos += inode_bitmap_block_num;


    // 配置inode表
    inode_table_block_pos_start = now_block_pos;
    if(fs->inode_table == NULL)
    {
        return -1;
    }
    memset(fs->inode_table, 0, sizeof(ext2_inode_t) * fs->super->inodes_count);
    inode_table_block_num = (sizeof(ext2_inode_t) * fs->super->inodes_count)/BLOCK_SIZE; //计算inode表所占的块数
    now_block_pos += inode_table_block_num;


    // 配置数据块
    data_block_pos_start = now_block_pos;
    data_block_num = fs->super->blocks_count - now_block_pos;


    // 配置group_descriptor
    fs->group->block_bitmap_start_idx = block_bitmap_block_pos_start;
    fs->group->inode_bitmap_start_idx = inode_bitmap_block_pos_start;
    fs->group->inode_table_start_idx = inode_table_block_pos_start;
    fs->group->data_block_start_idx = data_block_pos_start;


    printf("super_block_start_idx=%ld\n",super_block_pos_start);
    printf("block_bitmap_start_idx=%ld\n",fs->group->block_bitmap_start_idx);
    printf("inode_bitmap_start_idx=%ld\n",fs->group->inode_bitmap_start_idx);
    printf("inode_bitmap_block_num=%ld\n",inode_bitmap_block_num);
    
    printf("inode_table_start_idx=%ld\n",fs->group->inode_table_start_idx);
    printf("data_block_start_idx=%ld\n",fs->group->data_block_start_idx);


    // 把前面占用的block写入block_bitmap
    for (size_t i = super_block_pos_start; i < data_block_pos_start; i++)
    {
        bitmap_set_bit(fs->block_bitmap,i);
    }
    

    // 统一写入
    DISK_WRITE(fs->super,super_block_pos_start,super_block_num);
    DISK_WRITE(fs->group,group_block_pos_start,group_block_num);
    DISK_WRITE(fs->block_bitmap,block_bitmap_block_pos_start,block_bitmap_block_num);
    DISK_WRITE(fs->inode_bitmap,inode_bitmap_block_pos_start,inode_bitmap_block_num);
    DISK_WRITE(fs->inode_table,inode_table_block_pos_start,inode_table_block_num);

    return 0;
}

// int64_t ext2_fs_load(ext2_fs_t *fs)
// {

// }