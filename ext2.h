/**
 * @FilePath: /simple_file_system_test/ext2.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:59
 * @LastEditTime: 2025-06-05 16:27:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __EXT2_H__
#define __EXT2_H__

#include "stdint.h"

#define EXT2_SUPER_MAGIC 0xEF53

typedef struct ext2_fs ext2_fs_t;

extern ext2_fs_t* ext2_fs_create();
extern int64_t ext2_fs_format(ext2_fs_t *fs);
extern int64_t ext2_fs_load(ext2_fs_t *fs);
extern int64_t ext2_write_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size);
extern int64_t ext2_read_file(ext2_fs_t *fs, uint64_t inode_idx, void *buf);
#endif