/**
 * @FilePath: /simple_file_system_test/main.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-01 15:43:40
 * @LastEditTime: 2025-06-02 19:07:41
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "stdio.h"
#include "ext2.h"
#include "stddef.h"
#include "assert.h"

int main() {
    ext2_fs_t *fs = ext2_fs_create();
    assert(fs != NULL); 
    ext2_fs_format(fs);

    return 0;
}