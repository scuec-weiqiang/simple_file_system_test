/**
 * @FilePath: /simple_file_system_test/virtdisk.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-01 15:44:10
 * @LastEditTime: 2025-06-02 18:03:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef DISK_H
#define DISK_H

#include "stdint.h"

#define BLOCK_SIZE 512
#define DISK_SIZE 64*1024*1024

void  disk_read(uint8_t* buf, uint64_t sector);
void  disk_write(uint8_t* buf, uint64_t sector);

#define DISK_READ(buf, start,num) \
    do{\
        for(uint64_t sector = start;sector < (start+num);++sector) \
        {\
            disk_read((uint8_t*)buf, sector); \
        }\
    }while(0) \

#define DISK_WRITE(buf, start,num) \
    do{\
        for(uint64_t sector = start;sector < (start+num);++sector) \
        {\
            disk_write((uint8_t*)buf, sector); \
        }\
    }while(0) \

#endif