/**
 * @FilePath: /simple_file_system_test/main.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-01 15:43:40
 * @LastEditTime: 2025-06-25 01:35:57
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "stdio.h"
#include "ext2.h"
#include "stddef.h"
#include "assert.h"

#define BLOCK_SIZE 512
#define BLOCK_COUNT 4

int main()
{

    char *long_data = malloc(BLOCK_SIZE * BLOCK_COUNT);
    char *read = malloc(BLOCK_SIZE * BLOCK_COUNT);
// 填充重复数据
    for (int i = 0; i < BLOCK_SIZE * BLOCK_COUNT-100; ++i) {
        long_data[i] = 'A' ;
        long_data[i+1] = 0;
    }
    

    ext2_fs_t *fs = ext2_fs_create();
    assert(fs != NULL,printf("NULL prt\n");); 
    ext2_fs_format(fs);
    // ext2_fs_load(fs);
    
    uint64_t ret = 0;
    ext2_create_dir_by_path(fs, "/a/b/");
    ext2_create_file_by_path(fs, "/a/b/testfile.txt");
    ext2_append_file_by_path(fs, "/a/b/testfile.txt", long_data, BLOCK_SIZE * BLOCK_COUNT);
    // ext2_read_file_by_path(fs, "/a/b/testfile.txt", read);
    // printf("read file /a/b/testfile.txt:\n%s\n",read);

    ext2_unlink_by_path(fs, "/a/b/testfile.txt");
    ext2_unlink_by_path(fs, "/a/b");
    ext2_unlink_by_path(fs, "/a");


    ext2_create_dir_by_path(fs, "/a/b/");
    ext2_create_file_by_path(fs, "/a/b/testfile.txt");
    ext2_append_file_by_path(fs, "/a/b/testfile.txt", long_data, BLOCK_SIZE * BLOCK_COUNT);
    // ext2_read_file_by_path(fs, "/a/b/testfile.txt", read);
    // printf("read file /a/b/testfile.txt:\n%s\n",read);

    ext2_unlink_by_path(fs, "/a/b/testfile.txt");
    ext2_unlink_by_path(fs, "/a/b");
    ext2_unlink_by_path(fs, "/a");
    
    // ext2_read_file_by_path(fs, "/a/b/testfile.txt", read);
    // printf("read file /a/b/testfile.txt:\n%s\n",read);

    // ext2_create_file_by_path(fs, "/a/b/testfile.txt");
    // ext2_append_file_by_path(fs, "/a/b/testfile.txt", "123456789", 10);
    // ext2_read_file_by_path(fs, "/a/b/testfile.txt", read);
    // printf("read file /a/b/testfile.txt:\n%s\n",read);

    // ext2_delete_file_by_path(fs, "/a/b/testfile.txt");
    // ext2_delete_file_by_path(fs, "/a/b/");
    // ext2_delete_file_by_path(fs, "/a");


    return 0;
}