/**
 * @FilePath: /simple_file_system_test/bitmap.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 17:54:44
 * @LastEditTime: 2025-06-02 17:31:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BITMAP_H    
#define BITMAP_H

#include "stdint.h"
#include "stddef.h"

typedef struct bitmap bitmap_t;

bitmap_t* bitmap_create(size_t size);
int64_t bitmap_destory(bitmap_t **bm);

int64_t bitmap_set_bit(bitmap_t *bm, uint64_t index);
int64_t bitmap_clear_bit(bitmap_t *bm, uint64_t index);
int64_t bitmap_test_bit(bitmap_t *bm, uint64_t index);
size_t  bitmap_get_size(bitmap_t *bm);
size_t  bitmap_get_bytes_num(bitmap_t *bm);

#endif