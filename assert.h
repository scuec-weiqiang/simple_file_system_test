/**
 * @FilePath: /simple_file_system_test/assert.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-04 16:51:09
 * @LastEditTime: 2025-06-21 00:20:08
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _ASSERT_H
#define _ASSERT_H

 // 使用你的打印函数
#include "stdio.h" 

static inline void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
    printf("Assertion failed: %s, function %s, file %s, line %d\n", expr, func, file, line);
}

// #define NDEBUG

#ifdef NDEBUG
    #define assert(expr,ret) \
        do{ \
            if(!(expr)) ret;\
        }while(0)
#else
    #define assert(expr,ret) \
        do{ \
            if(!(expr))\
            {\
                __assert_fail(#expr, __FILE__, __LINE__, __func__); \
                ret;\
            }\
        }while(0)
#endif


#endif