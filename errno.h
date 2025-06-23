/**
 * @FilePath: /simple_file_system_test/errno.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:27:48
 * @LastEditTime: 2025-06-23 23:30:47
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ERRNO_H
#define _ERRNO_H

#define        SUCCESS  0// 成功
#define        FAILED  -1// 失败
#define        ERROR_NOT_FOUND  -2// 未找到匹配的数据
#define        ERROR_NOT_FREE  -3// 表/缓冲区已满
#define        ERROR_INDEX_OUT_OF_BOUNDS  -4// 标号索引越界
#define        ERROR_MEMORY_ALLOCATION  -5// 内存分配错误
#define        ERROR_MEMORY_FREE  -6// 内存释放错误
#define        ERROR_NULL_POINTER  -7// 空指针
#define        ERROR_TIMEOUT  -8// 超时
#define        ERROR_INVALID_ARG  -9// 无效参数错误
#define        ERROR_DUPLICATE  -10// 重复错误

    
#endif 