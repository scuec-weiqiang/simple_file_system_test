/**
 * @FilePath: /simple_file_system_test/ext2.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:54
 * @LastEditTime: 2025-06-25 01:38:00
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
#include "errno.h"

char *strdup(const char *s) {
    if (s == NULL) return NULL;
    
    size_t len = strlen(s) + 1;  // 计算长度（含结束符）
    char *dup = malloc(len);     // 分配内存
    
    if (dup != NULL) {
        strcpy(dup, s);          // 复制字符串
    }
    
    return dup;
}

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
#define MAX_BLK_NUM 13
    uint64_t blk_idx[MAX_BLK_NUM];  // 所在块号
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
    uint64_t cwd_inode_idx; // 当前工作目录的inode索引
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
    // 初始化当前工作目录为根目录
    fs->cwd_inode_idx = ROOT_INODE_IDX; 
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

    // printf("super_block_start_idx=%ld\n",super_block_pos_start);
    // printf("block_bitmap_start_idx=%ld\n",fs->group->block_bitmap_start_idx);
    // printf("inode_bitmap_start_idx=%ld\n",fs->group->inode_bitmap_start_idx);
    // printf("inode_table_start_idx=%ld\n",fs->group->inode_table_start_idx);
    // printf("data_block_start_idx=%ld\n",fs->group->data_block_start_idx);


    // 把前面占用的block写入block_bitmap
    for (size_t i = super_block_pos_start; i < data_block_pos_start; i++)
    {
        bitmap_set_bit(fs->block_bitmap,i);
        // printf("%d,blockbitmap = %lx\n",i,((uint64_t*)((uint64_t*)fs->block_bitmap)[0])[0] );
    }
    
    bitmap_set_bit(fs->inode_bitmap, ROOT_INODE_IDX); // 设置根目录的inode位图
    fs->super->free_inodes_count--; // 根目录的inode已经被分配
    fs->inode_table[ROOT_INODE_IDX].type = FILE_TYPE_DIR; // 第一个inode设为根目录
    fs->inode_table[ROOT_INODE_IDX].priv = 0; // 权限设置为0
    fs->inode_table[ROOT_INODE_IDX].size = 0; // 初始大小为0
    fs->inode_table[ROOT_INODE_IDX].ctime = 1; // 创建时间设为1
    // fs->inode_table[ROOT_INODE_IDX].blk_idx[0] = ext2_alloc_block(fs); // 分配一个数据块给根目录
    printf("free inode num = %ld\n", fs->super->free_inodes_count);
    printf("free block num = %ld\n\n", fs->super->free_blocks_count);
    // 统一写入
    DISK_WRITE(fs->super,super_block_pos_start,super_block_num);
    DISK_WRITE(fs->group,group_block_pos_start,group_block_num);
    DISK_WRITE(fs->block_bitmap,block_bitmap_block_pos_start,block_bitmap_block_num);
    DISK_WRITE(fs->inode_bitmap,inode_bitmap_block_pos_start,inode_bitmap_block_num);
    DISK_WRITE(fs->inode_table,inode_table_block_pos_start,inode_table_block_num);

    return 0;
}


/*
* @brief 加载 ext2 文件系统
*
* 该函数用于从磁盘加载 ext2 文件系统的超级块、组描述符、块位图、inode 位图和 inode 表。
*
* @param fs 指向 ext2 文件系统的指针
*
* @return 成功返回 0，失败返回 -1
*/
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


/**
 * @brief 分配一个新的块
 *
 * 该函数用于分配一个新的块，并更新文件系统的空闲块计数。
 *
 * @param fs 指向ext2文件系统的指针
 *
 * @return 成功返回新分配的块索引，失败返回-1。
 */
int64_t ext2_alloc_block(ext2_fs_t *fs)
{
    assert(fs!=NULL,return -1;);
    assert(fs->super->free_blocks_count>=0,return -1;);

    int64_t ret = bitmap_scan_0(fs->block_bitmap);
    
    if(ret<0)
    {
        printf("No free blocks available.\n");
        return ERROR_NOT_FREE; // 没有可用的块
    }
   
    bitmap_set_bit(fs->block_bitmap,ret);
    fs->super->free_blocks_count--;
    return ret;

}


/**
 * @brief 释放指定的块
 *
 * 该函数用于释放指定的块，并更新文件系统的空闲块计数。
 *
 * @param fs 指向ext2文件系统的指针
 * @param idx 要释放的块索引
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_free_block(ext2_fs_t *fs,uint64_t idx)
{
    assert(fs!=NULL,return -1;);
    int64_t ret = bitmap_clear_bit(fs->block_bitmap,idx);
    if(ret<0)
    {
        return FAILED; // 没有可用的块
    }
    fs->super->free_blocks_count++;
    return ret;
}


/**
 * @brief 分配一个新的inode
 *
 * 该函数用于分配一个新的inode，并更新文件系统的空闲inode计数。
 *
 * @param fs 指向ext2文件系统的指针
 *
 * @return 成功返回新分配的inode索引，失败返回-1。
 */
int64_t ext2_alloc_inode(ext2_fs_t *fs)
{
    assert(fs!=NULL,return -1;);
    assert(fs->super->free_inodes_count>=0,return -1;);

    int64_t ret =  bitmap_scan_0(fs->inode_bitmap);
    if(ret<0)
    {
        printf("No free inodes available.\n");
        return ret; // 没有可用的inode
    }
  
    bitmap_set_bit(fs->inode_bitmap,(uint64_t)ret);
    fs->super->free_inodes_count--;
    return ret;

}


/**
 * @brief 释放指定的inode
 *
 * 该函数用于释放指定的inode，并更新文件系统的空闲inode计数。
 *
 * @param fs 指向ext2文件系统的指针
 * @param idx 要释放的inode索引
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_free_inode(ext2_fs_t *fs,uint64_t idx)
{
    assert(fs!=NULL,return -1;);
    int64_t ret = bitmap_clear_bit(fs->inode_bitmap,idx);
    if(ret<0)
    {
        return FAILED;// 没有可用的inode
    }
    fs->super->free_inodes_count++;
    return ret;
}


/**
 * @brief 覆盖写数据到指定inode的文件
 *
 * 该函数用于将数据覆盖写到指定inode的文件中，并更新inode的大小和修改时间。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 要覆盖写数据的inode索引
 * @param data 要覆盖写的数据
 * @param size 要覆盖写的数据大小
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_overwrite_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size)
{
    assert(fs!=NULL,return -1;);
    
    assert(inode_idx<fs->super->inodes_count,return -1;);
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
    // disk_write(fs->inode_table, fs->group->inode_table_start_idx + inode_idx * sizeof(ext2_inode_t)/ BLOCK_SIZE);
    return 0;

}


/**
 * @brief 追加数据到指定inode的文件
 *
 * 该函数用于将数据追加到指定inode的文件中，并更新inode的大小和修改时间。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 要追加数据的inode索引
 * @param data 要追加的数据
 * @param size 要追加的数据大小
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_append_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->inodes_count,return -1;);
    assert(data!=NULL,return -1;);

    ext2_inode_t *dir_inode = &fs->inode_table[inode_idx];
    // 计算追加之后需要的块数
    uint64_t blocks_needed = (dir_inode->size + size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    assert(blocks_needed<=13&&blocks_needed<=fs->super->free_blocks_count,return -1;);

    uint64_t blocks_used = (dir_inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for(uint64_t i=blocks_used;i<blocks_needed;i++)
    {
        dir_inode->blk_idx[i] = ext2_alloc_block(fs); // 多出来的部分分配空间
    }

    for(uint64_t i = blocks_used;i<blocks_needed;i++)
    {
        uint64_t append_in_which_byte = dir_inode->size % BLOCK_SIZE; // 追加到这个块的哪个字节
        uint8_t *data_ptr = (uint8_t*)data;
        uint8_t temp_buf[BLOCK_SIZE];
        // 读取当前块内容
        disk_read(temp_buf, dir_inode->blk_idx[i]);
        
        // 将新目录项写入到当前块
        memcpy(temp_buf + append_in_which_byte,data_ptr, BLOCK_SIZE- append_in_which_byte);
        data_ptr += BLOCK_SIZE - append_in_which_byte; // 更新指针，指向下一个要写入的数据
        // 写回块
        disk_write(temp_buf, dir_inode->blk_idx[i]);
    }
        
    dir_inode->size += size;
    dir_inode->ctime++;
    // 更新目录的inode信息
    DISK_WRITE(fs->inode_table, fs->group->inode_table_start_idx, fs->group->inode_table_block_num);

    return 0;

}


/**
 * @brief 读取指定inode的文件内容
 *
 * 该函数用于从指定inode中读取文件内容，并将其存储到buf中。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 要读取的inode索引
 * @param buf 用于存储读取的文件内容的缓冲区
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_read_file(ext2_fs_t *fs, uint64_t inode_idx, void *buf)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->inodes_count,return -1;);
    assert(buf!=NULL,return -1;);

    uint64_t blocks_used = (fs->inode_table[inode_idx].size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for(uint64_t i = 0;i<blocks_used;i++)
    {
        disk_read((uint8_t*)buf+i*BLOCK_SIZE,fs->inode_table[inode_idx].blk_idx[i]);
    }
    return 0;
}


/**
 * @brief 删除指定inode的文件或目录
 *
 * 该函数用于删除指定inode的文件或目录，包括释放其占用的块和inode，并清空inode信息。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 要删除的inode索引
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_delete_inode_data(ext2_fs_t *fs, uint64_t inode_idx)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->inodes_count,return -1;);

    // 释放块
    for(uint64_t i = 0;i<13;i++)
    {
        if(fs->inode_table[inode_idx].blk_idx[i] != 0)
        {
            ext2_free_block(fs,fs->inode_table[inode_idx].blk_idx[i]);
        }
    }
    // 释放inode
    ext2_free_inode(fs,inode_idx);
    // 清空inode信息
    memset(&fs->inode_table[inode_idx], 0, sizeof(ext2_inode_t));
    
    DISK_WRITE(fs->inode_table, fs->group->inode_table_start_idx, fs->group->inode_table_block_num);
    
    return SUCCESS;
}


/**
 * @brief 获取指定目录下的目录项
 *
 * 该函数用于在指定的ext2文件系统中查找指定目录下的目录项，并将其信息存储在entry_ret中。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 目录的inode索引
 * @param name 要查找的文件名
 * @param entry_ret 用于存储找到的目录项信息
 *
 * @return 成功返回0，未找到返回-1。
 */
int64_t ext2_get_entry(ext2_fs_t *fs, uint64_t inode_idx, const char *name, ext2_dir_entry_t *entry_ret)
{
    assert(fs!=NULL,return ERROR_INVALID_ARG;);
    assert(name!=NULL,return ERROR_INVALID_ARG;);
    assert(inode_idx<fs->super->inodes_count,return ERROR_INVALID_ARG;);
    // assert(fs->inode_table[inode_idx].type == FILE_TYPE_DIR,return -1;);

    uint64_t blocks_used_num = (fs->inode_table[inode_idx].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint64_t all_entry_num =  fs->inode_table[inode_idx].size / sizeof(ext2_dir_entry_t);

    for(uint64_t i = 0; i < blocks_used_num; i++)
    {
        // 获取目录块索引，并读取该块的全部内容
        uint64_t blk_idx =  fs->inode_table[inode_idx].blk_idx[i];
        uint8_t entry_buf[BLOCK_SIZE];
        disk_read((uint8_t*)&entry_buf, blk_idx); 
        // 计算当前块中目录项的数量
        uint64_t block_entry_num = all_entry_num - i * (BLOCK_SIZE / sizeof(ext2_dir_entry_t));
        if(block_entry_num > BLOCK_SIZE / sizeof(ext2_dir_entry_t)) 
        {
            block_entry_num = BLOCK_SIZE / sizeof(ext2_dir_entry_t);
        }
        for(uint64_t i = 0; i < block_entry_num;i++)
        {
            ext2_dir_entry_t entry;
            entry = ((ext2_dir_entry_t*)entry_buf)[i]; // 读取目录项 
            // 检查文件名是否匹配
            if (strcmp(entry.name, name) == 0)
            {
                *entry_ret = entry; // 将找到的目录项复制到输出参数
                return SUCCESS; 
            }
        }
    }
    return ERROR_NOT_FOUND;
}


/**
 * @brief 在目录中查找文件或子目录
 *
 * 在指定的ext2文件系统中，查找指定目录下的文件名对应的inode索引。
 *
 * @param fs 指向ext2文件系统的指针
 * @param dir_inode_idx 目录的inode索引
 * @param name 要查找的文件名
 *
 * @return 如果找到匹配的文件名，则返回对应的inode索引；否则返回-1。
 */
int64_t ext2_find_entry(ext2_fs_t *fs, uint64_t inode_idx, const char *name)
{
    assert(fs!=NULL,return ERROR_INVALID_ARG;);
    assert(name!=NULL,return ERROR_INVALID_ARG;);
    assert(inode_idx<fs->super->inodes_count,return ERROR_INVALID_ARG;);
    // assert(fs->inode_table[inode_idx].type == FILE_TYPE_DIR,return -1;);

    ext2_dir_entry_t entry;
    int64_t ret = ext2_get_entry(fs, inode_idx, name, &entry);
    if(ret == SUCCESS) // 如果找到了目录项
    {
        return entry.inode_idx; // 返回对应的inode索引
    }
    else
    {
        // printf("Directory entry '%s' not found in inode %lu.\n", name, inode_idx);
        return ERROR_NOT_FOUND;
    }
    // 如果没有找到目录项

    
}


/**
 * @brief 初始化一个新的目录项
 *
 * 该函数用于初始化一个新的目录项，包括设置名称、inode索引和类型。
 *
 * @param fs 指向ext2文件系统的指针
 * @param new_entry 指向要初始化的目录项结构体
 * @param inode_idx 新目录项对应的inode索引
 * @param name 新目录项的名称
 * @param type 新目录项的类型（文件或目录）
 *
 * @return 成功返回0，失败返回-1。
 */
static int64_t ext2_init_entry(ext2_fs_t *fs, ext2_dir_entry_t *new_entry, uint64_t inode_idx, const char *name, uint32_t type)
{
    strncpy(new_entry->name, name, MAX_FILENAME_LEN);
    new_entry->name[MAX_FILENAME_LEN - 1] = '\0';
    new_entry->inode_idx =  inode_idx;// 分配一个新的inode
    fs->inode_table[inode_idx].type = type; // 设置新inode的类型
    fs->inode_table[inode_idx].priv = 0; // 设置新inode的权限为0
    fs->inode_table[inode_idx].size = 0; 
    return SUCCESS;
}


/**
 * @brief 在指定目录中添加一个新的目录项
 *
 * 该函数用于在指定的目录中添加一个新的目录项，但是不会检查目录项是否重复
 *
 * @param fs 指向ext2文件系统的指针
 * @param dir_inode_idx 目录的inode索引
 * @param name 要添加的目录项名称
 *
 * @return 成功添加返回新目录项的inode索引，失败返回-1。
 */
int64_t ext2_add_entry(ext2_fs_t *fs, uint64_t dir_inode_idx, const char *name,uint32_t type)
{
    assert(fs!=NULL,return -1;);
    assert(name!=NULL,return -1;);
    assert(dir_inode_idx<fs->super->inodes_count,return -1;);
    assert(fs->inode_table[dir_inode_idx].type == FILE_TYPE_DIR,return -1;);
    
    // 获取目录的inode信息
    ext2_inode_t *dir_inode = &fs->inode_table[dir_inode_idx];
    
    // 计算写入后需要的块数
    uint64_t blocks_needed = (dir_inode->size + sizeof(ext2_dir_entry_t) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 确保有足够的空间
    if(blocks_needed > 13) // 超过直接块数量，需要扩展
    {
        return -1; // 暂不支持超过13个直接块的情况
    }

    //遍历目录中所有的块，找到空位
    for(uint64_t i=0;i<MAX_BLK_NUM;i++)
    {
        if(dir_inode->blk_idx[i] == 0) // 如果没有分配块，则分配一个新的块
        {
            int64_t new_block_idx_ret = ext2_alloc_block(fs);
            if(new_block_idx_ret < 0) // 分配失败                       
            {
                return -1; // 分配块失败
            }
            dir_inode->blk_idx[i] = (uint64_t)new_block_idx_ret; // 更新块索引
        }

        ext2_dir_entry_t buffer[BLOCK_SIZE/sizeof(ext2_dir_entry_t)];
        for(uint64_t j=0;j<BLOCK_SIZE/sizeof(ext2_dir_entry_t);j++)
        {
            disk_read((uint8_t*)buffer,dir_inode->blk_idx[i]);
            if(buffer[j].inode_idx==0)//找到可用的位置
            {
                // 分配一个新的目录项
                ext2_dir_entry_t new_entry;
                uint64_t new_inode_idx;
                int64_t ret = ext2_alloc_inode(fs);
                if(ret < 0) // 分配inode失败
                {
                    printf("Failed to allocate inode.\n");
                    return ret; // 返回错误
                }
                new_inode_idx = (uint64_t)ret; // 获取新分配的inode索引
                ext2_init_entry(fs, &new_entry, new_inode_idx, name, type);

                
                buffer[j] = new_entry;
                disk_write((uint8_t*)buffer,dir_inode->blk_idx[i]);
                
                dir_inode->ctime++;
                dir_inode->size += sizeof(ext2_dir_entry_t);
                DISK_WRITE(fs->inode_table, fs->group->inode_table_start_idx, fs->group->inode_table_block_num);

                return new_entry.inode_idx;
            }
        }

    }
    return FAILED;
}


/**
 * @brief 删除指定目录中的目录项
 *
 * 该函数用于在指定的目录中删除一个目录项。它会查找目录项，如果找到则释放对应的inode并更新目录的创建时间和大小。
 *
 * @param fs 指向ext2文件系统的指针
 * @param dir_inode_idx 目录的inode索引
 * @param name 要删除的目录项名称
 *
 * @return 成功删除返回0，失败返回-1。
 */
int64_t ext2_remove_entry(ext2_fs_t *fs, uint64_t dir_inode_idx, const char *name)
{
    assert(fs!=NULL,return -1;);
    assert(name!=NULL,return -1;);
    assert(dir_inode_idx<fs->super->inodes_count,return -1;);
    assert(fs->inode_table[dir_inode_idx].type == FILE_TYPE_DIR,return -1;);

    ext2_dir_entry_t entry;
    // 获取要删除的entry信息
    int64_t ret = ext2_get_entry(fs, dir_inode_idx, name, &entry);
    if(ret != SUCCESS) // 如果获取目录项失败
    {
        printf("Failed to get directory entry.\n");
        return ret; // 返回错误
    }
    // 检查要删除的entry是否是目录
    if (fs->inode_table[entry.inode_idx].type == FILE_TYPE_DIR) 
    {
        if(entry.inode_idx == ROOT_INODE_IDX) // 如果是根目录
        {
            printf("Cannot remove root directory.\n");
            return -1; // 返回错误
        }
        if(fs->inode_table[entry.inode_idx].size > 0)
        {
            // 不能删除非空目录
            printf("Cannot remove non-empty directory.\n");
            return -1; // 返回错误
        }
    }
    
    ext2_delete_inode_data(fs, entry.inode_idx); // 删除inode数据
    
    entry.name[0] = '\0'; // 清空名称
    entry.inode_idx = 0; // 清空inode索引

    // 更新目录的创建时间和大小
    fs->inode_table[dir_inode_idx].ctime++;
    fs->inode_table[dir_inode_idx].size -= sizeof(ext2_dir_entry_t);

    DISK_WRITE(fs->inode_table, fs->group->inode_table_start_idx, fs->group->inode_table_block_num);
    
    return 0; // 成功删除
}


void print_entry(ext2_fs_t *fs,const ext2_dir_entry_t *entry) 
{
    // ANSI颜色代码定义
    #define COLOR_RESET   "\033[0m"      // 重置颜色
    #define COLOR_BLUE    "\033[1;34m"   // 蓝色（目录）
    #define COLOR_WHITE   "\033[1;37m"   // 白色（普通文件）
    #define COLOR_GREEN   "\033[1;32m"   // 绿色（可执行文件）
    
    if (entry->inode_idx == 0) {
        return; // 跳过无效的目录项
    }
    
    // 获取inode信息来判断文件类型
    ext2_inode_t *inode = &fs->inode_table[entry->inode_idx];
    
    // 根据文件类型设置颜色
    const char *color = COLOR_WHITE; // 默认白色
    const char *type_indicator = "";
    
    if (inode->type == FILE_TYPE_DIR) {
        color = COLOR_BLUE;
        type_indicator = "/";
    } else if (inode->type == FILE_TYPE_FILE) {
        color = COLOR_WHITE;
        type_indicator = "";
    }
    
    // 输出带颜色的文件名和类型指示符
    printf("%s%-20s%s%s  inode: %lu  size: %lu bytes\n", 
           color, entry->name, type_indicator, COLOR_RESET, 
           entry->inode_idx, inode->size);
}


/**
 * @brief 遍历指定目录中的所有目录项
 *
 * 该函数用于遍历指定目录中的所有目录项，并对每个目录项执行回调函数。
 *
 * @param fs 指向ext2文件系统的指针
 * @param dir_inode_idx 目录的inode索引
 * @param callback 回调函数，用于处理每个目录项
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_walk_entry(ext2_fs_t *fs, uint64_t dir_inode_idx, void (*callback)(ext2_fs_t *,const ext2_dir_entry_t *)) 
{
    assert(fs != NULL,return ERROR_INVALID_ARG;);
    ext2_inode_t *inode = &fs->inode_table[dir_inode_idx];

    if (inode->type != FILE_TYPE_DIR) {
        return -1;  // 不是目录
    }

    uint64_t entries_per_block = BLOCK_SIZE / sizeof(ext2_dir_entry_t);
    uint8_t block_buf[BLOCK_SIZE];

    for (uint64_t i = 0; i < MAX_BLK_NUM && inode->blk_idx[i]; i++) {
        disk_read(block_buf, inode->blk_idx[i]);
        ext2_dir_entry_t *entries = (ext2_dir_entry_t *)block_buf;

        for (uint64_t j = 0; j < entries_per_block; j++) {
            if (entries[j].inode_idx != 0) {
                callback(fs,&entries[j]);
            }
        }
    }

    return 0;
}


/**
 * @brief 获取指定inode的大小
 *
 * 该函数用于获取指定inode的大小（以字节为单位）。
 *
 * @param fs 指向ext2文件系统的指针
 * @param inode_idx 要获取大小的inode索引
 *
 * @return 返回inode的大小，如果发生错误则返回-1。
 */
int64_t ext2_get_inode_size(ext2_fs_t *fs, uint64_t inode_idx)
{
    assert(fs!=NULL,return -1;);
    assert(inode_idx<fs->super->inodes_count,return -1;);
    return fs->inode_table[inode_idx].size;
}


/**
 * @brief 将路径字符串分割成多个token
 *
 * 该函数用于将给定的路径字符串分割成多个token，每个token表示路径中的一个部分。
 * 如果传入的字符串为NULL，则继续从上次分割的位置开始分割。
 *
 * @param str 要分割的路径字符串，如果为NULL则继续上次分割的位置
 *
 * @return 返回下一个token的起始位置，如果没有更多token则返回NULL。
 */
char* ext2_path_split(char *str,const char *delim)
{
    static char* cur_token = NULL; // 用于保存下一个token的起始位置 
    static char* end = NULL; // 用于保存字符串结束位置
    
    if(str != NULL)
    {
        cur_token = str; // 保存当前token的起始位置
        end = str;
        // 如果str不为NULL，说明是第一次调用
        for(int i = 0; str[i] != '\0'; i++)
        {
            if(str[i] == delim[0]) // 找到路径分隔符
            {
                str[i] = '\0'; // 将分隔符替换为字符串结束符
            }
            end++; // 更新end指针，直到指向字符串的末尾
        }

    }
    else
    {
        // 如果str为NULL，说明是后续调用
        while(*cur_token != '\0') //这时cur_token指向上一个token的开头，需要跳过上一段token指向下一个/0分隔符
        {
            if(cur_token == end) // 如果
            {
                cur_token = NULL;
                break; // 如果已经到达字符串末尾，返回NULL
            }
            cur_token++;
           
        }
    }

    while(*cur_token == '\0') //这时cur_token已经不再指向上一个token的内容，但是有可能上个token结尾有多个/0分隔符，需要跳过这些分隔符指向下一个token开头
    {
        // 注意，一定要先判断是否到达字符串末尾，否则不会及时跳出循环，会越界报错
        if(cur_token == end) 
        {
            cur_token = NULL;
            break; // 如果已经到达字符串末尾，返回NULL
        }
        cur_token++;
    }
    
    return cur_token; 
}


/**
 * @brief 根据路径查找或创建对应的inode
 *
 * 该函数用于根据给定的路径查找对应的inode，如果不存在则根据auto_create参数决定是否自动创建目录。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要查找的路径字符串
 * @param auto_create 是否允许自动创建目录（0表示不允许，1表示允许）
 *
 * @return 返回找到的inode索引，如果没有找到且不允许自动创建则返回-1。
 */
static int64_t ext2_find_inode_by_path(ext2_fs_t *fs,const char *path,uint64_t auto_create)
{
    char *copy_path = strdup(path);
    int64_t parent_inode_idx = 0;
    int64_t child_inode_idx = 0;
    char* token = ext2_path_split(copy_path,"/");
    while(token) 
    {
        child_inode_idx = ext2_find_entry(fs, parent_inode_idx, token);
        if(child_inode_idx < 0) // 如果没有找到父目录,那就建立对应目录
        {
            if(auto_create == 0) // 如果不允许自动创建目录
            {
                printf("Directory %s not found.\n", token);
                free(copy_path); // 释放复制的路径字符串
                return -1; // 返回错误
            }
            else
            {
                child_inode_idx = ext2_add_entry(fs, parent_inode_idx, token, FILE_TYPE_DIR); // 在父目录下添加新的子目录
                if(child_inode_idx < 0) // 如果添加目录失败
                {
                    return -1; // 返回错误
                }
            }
           
        }
        parent_inode_idx = child_inode_idx; // 更新父目录的inode索引
        token = ext2_path_split(NULL,"/");
    }
    free(copy_path); // 释放复制的路径字符串
    return parent_inode_idx; // 返回最后找到的inode索引
}


/**
 * @brief 获取路径的基本名称
 *
 * 该函数用于从给定的路径字符串中提取基本名称（即最后一个斜杠后的部分）。
 *
 * @param path 要处理的路径字符串
 *
 * @return 返回指向基本名称的指针，如果路径中没有斜杠，则返回整个路径字符串。
 */
int64_t ext2_get_path_basename (char *path,char* basename)
{
    if (!path || !*path) return FAILED;

    size_t len = strlen(path);
    if(len> MAX_FILENAME_LEN-1) 
    {
        printf("Path length exceeds maximum allowed length.\n");
        return FAILED; // 路径长度超过最大限制
    }
    uint64_t basename_head = 0;
    uint64_t basename_tail = len;

    // 去掉尾部的 '/'（如果有）
    while (basename_tail > 1 && path[basename_tail - 1] == '/') 
    {
        basename_tail--;
    }
    // 向前查找最后一个 '/'
    for (int64_t i = basename_tail - 1; i >= 0; i--)
    {
        if (path[i] == '/') 
        {
            basename_head = i + 1;
            break;
        }
    }
    
    strncpy(basename, path + basename_head, basename_tail - basename_head);
    basename[basename_tail - basename_head] = '\0';

    return SUCCESS; 
    
}


/**
 * @brief 获取路径的目录名
 *
 * 该函数用于从给定的路径字符串中提取目录名（即最后一个斜杠之前的部分）。
 *
 * @param path 要处理的路径字符串
 * @param dirname 用于存储目录名的缓冲区
 *
 * @return 返回0表示成功，-1表示失败。
 */
int64_t ext2_get_path_dirname (char *path,char* dirname)
{
    if (!path || !*path) return FAILED;

    size_t len = strlen(path);
    if(len> MAX_FILENAME_LEN-1) 
    {
        printf("Path length exceeds maximum allowed length.\n");
        return FAILED; // 路径长度超过最大限制
    }

    uint64_t dirname_tail = len;

    // 去掉尾部的 '/'（如果有）
    while (dirname_tail > 1 && path[dirname_tail - 1] == '/') 
    {
        dirname_tail--;
    }
    // 向前查找最后一个 '/'
    for (int64_t i = dirname_tail - 1; i >= 0; i--)
    {
        if (path[i] == '/') 
        {
            dirname_tail = i + 1;
            break;
        }
    }
    
    strncpy(dirname, path, dirname_tail);
    dirname[dirname_tail] = '\0';

    return SUCCESS; 
    
}


/**
 * @brief 根据路径创建目录
 *
 * 该函数用于根据给定的路径创建一个新的目录。如果路径是根目录，则不需要创建。
 * 如果路径不存在，则会自动创建对应的目录。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要创建的目录路径
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_create_dir_by_path(ext2_fs_t *fs, const char *path)
{
    assert(fs!=NULL&&path!=NULL,return -1;);

    if(strcmp(path, "/") == 0) // 如果路径是根目录
    {
        return 0; // 根目录不需要创建
    }
    
    if(ext2_find_inode_by_path(fs, path, 1)<0) // 查找路径对应的inode，如果不存在则创建
    {
        printf("Failed to create directory: %s\n", path);
        return -1; // 返回错误
    }
    printf("Directory created successfully: %s\n", path);

    return 0; // 成功创建目录
}


/**
 * @brief 根据路径创建文件
 *
 * 该函数用于根据给定的路径创建一个新的文件。如果路径对应的目录不存在，则会自动创建目录。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要创建的文件路径
 *
 * @return 成功返回新创建文件的inode索引，失败返回-1。
 */
int64_t ext2_create_file_by_path(ext2_fs_t *fs, const char *path)
{
    assert(fs!=NULL&&path!=NULL,return -1;);

    char base_name[MAX_FILENAME_LEN]; 
    ext2_get_path_basename (path, base_name);
    char dir_name[MAX_FILENAME_LEN]; 
    ext2_get_path_dirname (path, dir_name);

    int64_t dir_inode_idx = ext2_find_inode_by_path(fs, dir_name, 0); // 查找路径对应的inode
    if( dir_inode_idx < 0)
    {
        printf("Failed to find for file: %s\n", path);
        return -1; // 返回错误
    }
    int64_t file_inode_idx = ext2_add_entry(fs,  dir_inode_idx, base_name, FILE_TYPE_FILE);
    if(file_inode_idx < 0) // 如果添加文件失败
    {
        printf("Failed to create file: %s\n", base_name);
        return -1; // 返回错误
    }

    printf("File created successfully: %s\n", path);

    return file_inode_idx; // 返回新创建文件的inode索引
}


/**
 * @brief 根据路径删除文件
 *
 * 该函数用于根据给定的路径删除一个文件。如果文件不存在，则返回错误。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要删除的文件路径
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_unlink_by_path(ext2_fs_t *fs, const char *path)
{
    assert(fs!=NULL&&path!=NULL,return -1;);
    char base_name[MAX_FILENAME_LEN]; 
    ext2_get_path_basename (path, base_name);
    char dir_name[MAX_FILENAME_LEN]; 
    ext2_get_path_dirname (path, dir_name);

    int64_t dir_inode_idx = ext2_find_inode_by_path(fs, dir_name, 0); // 查找路径对应的inode
    if( dir_inode_idx < 0)
    {
        printf("Failed to find for file: %s\n", path);
        return -1; // 返回错误
    }
    int64_t ret = ext2_remove_entry(fs, (uint64_t)dir_inode_idx, base_name);
    if(ret < 0) // 如果添加文件失败
    {
        printf("Failed to delete file: %s\n", base_name);
        return ret; // 返回错误
    }

    printf("File deleted successfully: %s\n", path);
 
    return SUCCESS; // 成功删除文件
}


/**
 * @brief 追加数据到指定路径的文件
 *
 * 该函数用于将数据追加到指定路径的文件中。如果文件不存在，则返回错误。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要追加数据的文件路径
 * @param data 要追加的数据
 * @param size 要追加的数据大小
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_append_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size)
{
    assert(fs!=NULL&&path!=NULL&&data!=NULL,return -1;);
    int64_t inode_idx = ext2_find_inode_by_path(fs, path, 0); // 查找路径对应的inode
    if(inode_idx<0)
    {
        printf("Failed to find for file: %s\n", path);
        return -1; // 返回错误
    }

    if(ext2_append_file(fs, inode_idx, data, size)<0) // 如果追加文件失败
    {
        printf("Failed to write file: %s\n", path);
        return -1; // 返回错误
    }
    printf("File written successfully: %s\n", path);
    
    return 0;
}


/**
 * @brief 覆盖写数据到指定路径的文件
 *
 * 该函数用于将数据覆盖写到指定路径的文件中。如果文件不存在，则返回错误。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要覆盖写数据的文件路径
 * @param data 要覆盖写的数据
 * @param size 要覆盖写的数据大小
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_overwrite_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size)
{
    assert(fs!=NULL&&path!=NULL&&data!=NULL,return -1;);
    int64_t inode_idx = ext2_find_inode_by_path(fs, path, 0); // 查找路径对应的inode
    if(inode_idx<0)
    {
        printf("Failed to find for file: %s\n", path);
        return -1; // 返回错误
    }

    if(ext2_overwrite_file(fs, inode_idx, data, size)<0) // 如果覆盖文件失败
    {
        printf("Failed to write file: %s\n", path);
        return -1; // 返回错误
    }
    printf("File written successfully: %s\n", path);
    return 0;
}


/**
 * @brief 根据路径读取文件内容
 *
 * 该函数用于根据给定的路径读取文件内容，并将其存储到buf中。如果文件不存在，则返回错误。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要读取的文件路径
 * @param buf 用于存储读取的文件内容的缓冲区
 *
 * @return 成功返回0，失败返回-1。
 */
int64_t ext2_read_file_by_path(ext2_fs_t *fs, const char *path, void *buf)
{
    assert(fs!=NULL&&path!=NULL,return -1;);
    int64_t inode_idx = ext2_find_inode_by_path(fs, path, 0); // 查找路径对应的inode
    if(inode_idx<0)
    {
        printf("Failed to find or create directory for file: %s\n", path);
        return -1; // 返回错误
    }

    if(ext2_read_file(fs, inode_idx, buf)<0) // 如果读取文件失败
    {
        printf("Failed to read file: %s\n", path);
     
        return -1; // 返回错误
    }
    printf("File read successfully: %s\n", path);
    return 0;
}



int64_t ext2_list_dir_by_path(ext2_fs_t *fs,const char *path)
{
    assert(fs!=NULL&&path!=NULL,return ERROR_INVALID_ARG;);
    int64_t inode_idx = ext2_find_inode_by_path(fs, path, 0); // 查找路径对应的inode
    if(inode_idx<0)
    {
         printf("Failed to find directory: %s\n", path);
        return -1; // 返回错误
    }
    int64_t ret = ext2_walk_entry(fs, inode_idx, print_entry);
    return ret;
}


/**
 * @brief 根据路径获取文件的大小
 *
 * 该函数用于根据给定的路径获取对应文件的大小（以字节为单位）。
 *
 * @param fs 指向ext2文件系统的指针
 * @param path 要获取大小的文件或目录路径
 *
 * @return 返回文件的大小，如果发生错误则返回-1。
 */
int64_t ext2_get_inode_size_by_path(ext2_fs_t *fs, const char *path)
{

}

