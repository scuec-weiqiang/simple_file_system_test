// lib/assert.c
#include "assert.h"
#include "stdio.h"  // 使用你的内核打印函数

void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
    printf("Assertion failed: %s, function %s, file %s, line %d\n", expr, func, file, line);
	while (1)
	{
			/* code */
	}
       
//     panic("Assertion failed");
}