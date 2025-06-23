/**
 * @FilePath: /simple_file_system_test/ext2.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:59
 * @LastEditTime: 2025-06-20 23:56:32
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

extern int64_t ext2_overwrite_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size);
extern int64_t ext2_append_file(ext2_fs_t *fs, uint64_t inode_idx, const void *data, uint64_t size);
extern int64_t ext2_read_file(ext2_fs_t *fs, uint64_t inode_idx, void *buf);
extern int64_t ext2_find_entry(ext2_fs_t *fs, uint64_t inode_idx, const char *name);
extern int64_t ext2_add_entry(ext2_fs_t *fs, uint64_t dir_inode_idx, const char *name,uint32_t type);
extern int64_t ext2_get_file_size(ext2_fs_t *fs, uint64_t inode_idx);
extern char* ext2_path_split(char *str,const char *delim); // 路径分割函数
extern char* ext2_get_filename(char *path);

extern int64_t ext2_create_dir_by_path(ext2_fs_t *fs, const char *path);// 创建目录
extern int64_t ext2_create_file_by_path(ext2_fs_t *fs, const char *path); // 创建文件
extern int64_t ext2_write_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size); // 覆盖写
extern int64_t ext2_append_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size); // 追加写
extern int64_t ext2_read_file_by_path(ext2_fs_t *fs, const char *path, void *buf); // 读取
extern int64_t ext2_get_file_size_by_path(ext2_fs_t *fs, const char *path); // 查询文件大小
#endif