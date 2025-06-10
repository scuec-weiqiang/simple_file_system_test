/**
 * @FilePath: /simple_file_system_test/main.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-01 15:43:40
 * @LastEditTime: 2025-06-05 17:10:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "stdio.h"
#include "ext2.h"
#include "stddef.h"
#include "assert.h"

#define BLOCK_SIZE 512
#define BLOCK_COUNT 4

int main() {
    char *long_data = malloc(BLOCK_SIZE * BLOCK_COUNT);
    char *read = malloc(BLOCK_SIZE * BLOCK_COUNT);
// 填充重复数据
    for (int i = 0; i < BLOCK_SIZE * BLOCK_COUNT; ++i) {
        long_data[i] = 'A' + (i % 26); // ABCD...XYZ 循环
    }
    long_data[2047] = 0;

    ext2_fs_t *fs = ext2_fs_create();
    assert(fs != NULL,printf("NULL prt\n");); 
    ext2_fs_format(fs);
    // ext2_fs_load(fs);
    ext2_write_file(fs, 68, long_data, BLOCK_SIZE * BLOCK_COUNT);
    ext2_read_file(fs,68,read);
    printf("%s\n",read);
   
    ext2_write_file(fs, 68, "AB", 2);
    ext2_read_file(fs,68,read);
    printf("%s\n",read);
    
    return 0;
}