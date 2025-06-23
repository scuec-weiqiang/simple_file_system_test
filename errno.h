/**
 * @FilePath: /simple_file_system_test/errno.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:27:48
 * @LastEditTime: 2025-06-21 02:12:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ERRNO_H
#define _ERRNO_H

   typedef enum
    {
        SUCCESS = 0,// 成功
        FAILED = -1,// 失败
        ERROR_NOT_FOUND_ = -2,// 未找到匹配的数据
        ERROR_NOT_FREE = -3,// 表/缓冲区已满
        ERROR_INDEX_OUT_OF_BOUNDS = -4,// 标号索引越界
        ERROR_MEMORY_ALLOCATION = -5,// 内存分配错误
        ERROR_MEMORY_FREE = -6,// 内存释放错误
        ERROR_NULL_POINTER = -7,// 空指针
        ERROR_TIMEOUT = -8,// 超时
        ERROR_INVALID_ARG = -9,// 无效参数错误
    }status_t;



    
#endif 