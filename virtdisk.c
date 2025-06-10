/**
 * @FilePath: /simple_file_system_test/virtdisk.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-01 15:44:17
 * @LastEditTime: 2025-06-05 16:44:48
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "stdint.h"
#include "string.h"
#include "virtdisk.h"
#include "stdio.h"
uint8_t hard_disk[DISK_SIZE];//64m虚拟磁盘

void  disk_read(uint8_t* buf, uint64_t sector)
{   
    memcpy(buf, hard_disk + 512 * sector, 512);
}

void  disk_write(uint8_t* buf, uint64_t sector)
{
    memcpy(hard_disk + 512 * sector, buf, 512);
    // for(uint64_t i=0;i<2048;i++)
    // {
    //     printf("1%c",hard_disk[68*BLOCK_SIZE+i]);
    // }
}