/**
 * @FilePath: /simple_file_system_test/ext2.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:54
 * @LastEditTime: 2025-06-08 22:34:15
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

#define EXT2_INODE_DENSITY_PER_GIB 2048


typedef struct ext2_super_block {
#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_SUPER_BLOCK_IDX 0
    uint64_t magic;
    uint64_t inodes_count;      // inode总数
    uint64_t free_inodes_count; // 空闲inode数
    uint64_t blocks_count;      // 块总数
    uint64_t free_blocks_count; // 空闲块数
    uint64_t block_size;    // 块大小(字节)
    // ... 其他字段
}ext2_super_block_t;

typedef struct ext2_group_descriptor {
#define EXT2_GROUP_DESCRIPTOR_IDX 1
    uint64_t block_bitmap_start_idx;     // 块位图起始索引
    uint64_t block_bitmap_block_num;

    uint64_t inode_bitmap_start_idx;     // inode位图起始索引
    uint64_t inode_bitmap_block_num;

    uint64_t inode_table_start_idx;    // inode表起始索引
    uint64_t inode_table_block_num;

    uint64_t data_block_start_idx;  // 第一个数据块起始索引
    uint64_t data_block_num;

    uint64_t root_inode_idx;
}ext2_group_descriptor_t;

typedef struct ext2_inode {
#define ROOT_INODE_IDX 0

#define FILE_TYPE_DIR 1
#define FILE_TYPE_FILE 2
    uint32_t type;        // 文件类型
    uint32_t priv;        // 权限
    uint64_t size;        // 文件大小(字节)
    uint64_t ctime;       // 创建时间
#define MAX_BLK_IDX 13
    uint64_t blk_idx[MAX_BLK_IDX];  // 所在块号
}ext2_inode_t;


typedef struct ext2_dir_entry {
#define MAX_FILENAME_LEN 120 
    char name[MAX_FILENAME_LEN];  // 文件名
    uint64_t inode_idx;           // inode索引
} ext2_dir_entry_t;

typedef struct ext2_fs
{
    ext2_super_block_t *super; 
    ext2_group_descriptor_t *group; 
    bitmap_t *block_bitmap; 
    bitmap_t *inode_bitmap; 
    ext2_inode_t *inode_table; 
}ext2_fs_t;

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
    assert(fs!=NULL,return NULL);
   
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

    uint64_t super_block_pos_start = 0,super_block_num = 0;
    uint64_t group_block_pos_start = 0,group_block_num = 0;
    uint64_t block_bitmap_block_pos_start = 0,block_bitmap_block_num = 0;
    uint64_t inode_bitmap_block_pos_start = 0,inode_bitmap_block_num = 0;
    uint64_t inode_table_block_pos_start = 0,inode_table_block_num = 0;
    uint64_t data_block_pos_start = 0,data_block_num=0;

    // 配置super_block
    super_block_pos_start = now_block_pos;
    super_block_num = 1;
    assert(fs->super!=NULL,return -1);
    fs->super->magic = EXT2_SUPER_MAGIC; 
    fs->super->free_blocks_count = DISK_SIZE / fs->super->block_size;
    fs->super->free_inodes_count = 128;
    now_block_pos += super_block_num;


    // 配置group_descriptor
    group_block_pos_start = now_block_pos; //记录group的位置
    group_block_num = 1;
    //不急着配置group_descriptor,先预留空间,因为他的值要和后面的位图和inode表的位置有关
    assert(fs->group!=NULL,return -1);
    now_block_pos += group_block_num;


    // 配置两个位图
    block_bitmap_block_pos_start = now_block_pos;
    assert(fs->block_bitmap!=NULL,return -1);
    block_bitmap_block_num = (bitmap_get_bytes_num(fs->block_bitmap)+511)/BLOCK_SIZE; //计算块位图所占的块数
    now_block_pos += block_bitmap_block_num;

    inode_bitmap_block_pos_start = now_block_pos;
    assert(fs->inode_bitmap!=NULL,return -1);
    inode_bitmap_block_num = (bitmap_get_bytes_num(fs->inode_bitmap)+511)/BLOCK_SIZE; //计算inode位图所占的块数
    now_block_pos += inode_bitmap_block_num;


    // 配置inode表
    inode_table_block_pos_start = now_block_pos;
    assert(fs->inode_table!=NULL,return -1);
    memset(fs->inode_table, 0, sizeof(ext2_inode_t) * fs->super->inodes_count);
    inode_table_block_num = (sizeof(ext2_inode_t) * fs->super->inodes_count)/BLOCK_SIZE; //计算inode表所占的块数
    fs->inode_table[ROOT_INODE_IDX].type = FILE_TYPE_DIR; // 第一个inode设为根目录
    now_block_pos += inode_table_block_num;


    // 配置数据块
    data_block_pos_start = now_block_pos;
    data_block_num = fs->super->blocks_count - now_block_pos;

    // 配置group_descriptor
    fs->group->block_bitmap_start_idx = block_bitmap_block_pos_start;
    fs->group->block_bitmap_block_num = block_bitmap_block_num;
    fs->group->inode_bitmap_start_idx = inode_bitmap_block_pos_start;
    fs->group->inode_bitmap_block_num = inode_bitmap_block_num;
    fs->group->inode_table_start_idx = inode_table_block_pos_start;
    fs->group->inode_table_block_num = inode_table_block_num;
    fs->group->data_block_start_idx = data_block_pos_start;
    fs->group->data_block_num = data_block_num;

    printf("super_block_start_idx=%ld\n",super_block_pos_start);
    printf("block_bitmap_start_idx=%ld\n",fs->group->block_bitmap_start_idx);
    printf("inode_bitmap_start_idx=%ld\n",fs->group->inode_bitmap_start_idx);
    printf("inode_table_start_idx=%ld\n",fs->group->inode_table_start_idx);
    printf("data_block_start_idx=%ld\n",fs->group->data_block_start_idx);


    // 把前面占用的block写入block_bitmap
    for (size_t i = super_block_pos_start; i < data_block_pos_start; i++)
    {
        bitmap_set_bit(fs->block_bitmap,i);
        // printf("%d,blockbitmap = %lx\n",i,((uint64_t*)((uint64_t*)fs->block_bitmap)[0])[0] );
    }
    
    // 统一写入
    DISK_WRITE(fs->super,super_block_pos_start,super_block_num);
    DISK_WRITE(fs->group,group_block_pos_start,group_block_num);
    DISK_WRITE(fs->block_bitmap,block_bitmap_block_pos_start,block_bitmap_block_num);
    DISK_WRITE(fs->inode_bitmap,inode_bitmap_block_pos_start,inode_bitmap_block_num);
    DISK_WRITE(fs->inode_table,inode_table_block_pos_start,inode_table_block_num);

    return 0;
}

int64_t ext2_fs_load(ext2_fs_t *fs)
{
    assert(fs!=NULL,return -1;);

    DISK_READ(fs->super,EXT2_SUPER_BLOCK_IDX,1);
    DISK_READ(fs->group,EXT2_GROUP_DESCRIPTOR_IDX,1);

    // printf("magic = %x\n",fs->super->magic);
    // printf("free_inodes_count = %d\n",fs->super->free_inodes_count);

    DISK_READ(fs->block_bitmap,fs->group->block_bitmap_start_idx,fs->group->block_bitmap_block_num);
    DISK_READ(fs->inode_bitmap,fs->group->inode_bitmap_start_idx,fs->group->inode_bitmap_block_num);
    DISK_READ(fs->inode_table,fs->group->inode_table_start_idx,fs->group->inode_table_block_num);

}

int64_t ext2_alloc_block(ext2_fs_t *fs)
{
    assert(fs!=NULL,return -1;);
    assert(fs->super->free_blocks_count>=0,return -1;);

    uint64_t ret =  bitmap_scan_0(fs->block_bitmap);
    
    if(ret>=0)
    {
        bitmap_set_bit(fs->block_bitmap,ret);
        fs->super->free_blocks_count--;
    }
    return ret;

}

int64_t ext2_free_block(ext2_fs_t *fs,uint64_t idx)
{
    assert(fs!=NULL,return -1;);
    uint64_t ret = bitmap_clear_bit(fs->block_bitmap,idx);
    if(ret>=0)
    {
        fs->super->free_blocks_count++;
    }
    return ret;
}


int64_t ext2_write_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->blocks_count && inode_idx>=fs->group->data_block_start_idx,return -1;);
    assert(data!=NULL,return -1;);

    uint64_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    assert(blocks_needed<=13&&blocks_needed<=fs->super->free_blocks_count,return -1;);

    uint64_t blocks_used = (fs->inode_table[inode_idx].size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if(blocks_needed <= blocks_used) //
    {
        for(uint64_t i = blocks_needed;i<blocks_used;i++) // 将多余的空间释放
        {
            ext2_free_block(fs,fs->inode_table[inode_idx].blk_idx[i]);
        }
    }
    else
    {
        for(uint64_t i=blocks_used;i<blocks_needed;i++)
        {
            fs->inode_table[inode_idx].blk_idx[i] = ext2_alloc_block(fs); // 多出来的部分分配空间
        }
    }

    for(uint64_t i = 0;i<blocks_needed;i++)
    {
        disk_write((uint8_t*)data+i*BLOCK_SIZE,fs->inode_table[inode_idx].blk_idx[i]);
    }

    fs->inode_table[inode_idx].size = size;
    fs->inode_table[inode_idx].ctime++;

    DISK_WRITE(fs->inode_table, fs->group->inode_table_start_idx, fs->group->inode_table_block_num);

    return 0;

}


int64_t ext2_read_file(ext2_fs_t *fs, uint64_t inode_idx, void *buf)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->blocks_count && inode_idx>=fs->group->data_block_start_idx,return -1;);
    assert(buf!=NULL,return -1;);

    uint64_t blocks_used = (fs->inode_table[inode_idx].size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for(uint64_t i = 0;i<blocks_used;i++)
    {
        disk_read((uint8_t*)buf+i*BLOCK_SIZE,fs->inode_table[inode_idx].blk_idx[i]);
        // printf("%s \n",(uint8_t*)buf+i*BLOCK_SIZE);
    }

    printf("blocks_used = %ld\n",blocks_used);
    printf("size = %ld\n",fs->inode_table[inode_idx].size);
    printf("free blocks = %ld\n",fs->super->free_blocks_count);
    return 0;
}

int64_t ext2_find_entry(ext2_fs_t *fs, uint64_t dir_inode_idx, const char *name)
{
    assert(fs!=NULL,return -1;);
    assert(name!=NULL,return -1;);
    
    char filename[MAX_FILENAME_LEN];
    uint64_t i = 0;
    while(name[i]!=0)
    {
        
    }

}

int64_t ext2_create_file(const char* path)
{
   
}