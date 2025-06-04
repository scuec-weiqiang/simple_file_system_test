/**
 * @FilePath: /simple_file_system_test/ext2.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:59
 * @LastEditTime: 2025-06-03 23:45:39
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __EXT2_H__
#define __EXT2_H__

#include "stdint.h"

#define EXT2_SUPER_MAGIC 0xEF53

typedef struct ext2_fs ext2_fs_t;

extern uint64_t super_block_pos_start,super_block_num;
extern uint64_t group_block_pos_start,group_block_num;
extern uint64_t block_bitmap_block_pos_start,block_bitmap_block_num;
extern uint64_t inode_bitmap_block_pos_start,inode_bitmap_block_num;
extern uint64_t inode_table_block_pos_start,inode_table_block_num;
extern uint64_t data_block_pos_start,data_block_num;

extern ext2_fs_t* ext2_fs_create();
extern int64_t ext2_fs_format(ext2_fs_t *fs);


#endif