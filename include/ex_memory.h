#ifndef EX_MEMORY_H_201410021810
#define EX_MEMORY_H_201410021810
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   ex_memory.h
 *      Description :
 *      Created     :   2014-10-02 18:10:39
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#define ex_arraysize(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @Brief  释放内存，简化free写法
 *         原写法：
 *          if(NULL == buf){
 *              free(buf);
 *              buf = NULL;
 *          }
 *          现写法: ex_free(buf);
 *
 * @Param __free_buf
 *
 * @Returns
 */
#define ex_free(ptr) 	\
	do{\
		if(NULL != (ptr)){\
			free(ptr);	\
			(ptr) = NULL;	\
		}\
	}while(0)

#undef ex_delete
#define ex_delete(ptr) \
    do{\
        if(NULL != ptr){\
            delete ptr;\
        }\
        ptr = NULL;\
    }while(0)

#define ex_del_array(ptr) \
    do{\
        if(NULL != ptr){\
            delete[] ptr;\
        }\
        ptr = NULL;\
    }while(0)

/**
 * @Brief  释放二级malloc后的内存
 *         例子： 释放内存 float **data = MALLOC(float*, 4);
 *                         data[i]也已经分配对应的内存
 *                调用： FREE_ARRAY(data, 4, i);
 *
 * @Param __free_buf_array      要释放的内存地址
 * @Param __free_buf_array_num  要释放的内存下一级内存的数量
 * @Param __array_idx           用来遍历下一级内存的变量
 *
 * @Returns
 */
#define  ex_free_array(__free_buf_array, __free_buf_array_num, __array_idx)   \
    do{\
        for (__array_idx = 0; __array_idx < __free_buf_array_num; __array_idx++) {\
            if(NULL != __free_buf_array) ex_free(__free_buf_array[__array_idx]);\
        }\
        ex_free(__free_buf_array);\
    }while(0)

void *Malloc(size_t size);
/**
 * @Brief  简化malloc函数的写法
 *         用例:
 *         分配4个float大小的内存，
 *         原写法       float *data = (float *)malloc(sizeof(float) * 4);
 *         现在写法:    float *data = ex_malloc(float, 4);
 *
 *         分配4个float*内存，
 *         原写法       float **data = (float **)malloc(sizeof(float*) * 4);
 *         现在写法:    float **data = ex_malloc(float*, 4);
 *
 *         注意事项：
 *              在分配内存的数据类型为指针类型时，数据类型和*号之间不能有空格
 *
 *
 * @Param __data_type       要分配的数据类型
 * @Param __data_size       要分配的数据大小，单位sizeof(__data_type)
 *
 * @Returns     分配后的内存地址
 */
#define ex_malloc(__data_type, __data_cnt)  \
            (__data_type*)Malloc(sizeof(__data_type) * (__data_cnt))
#define ex_malloc_one(__data_type)  (__data_type*)Malloc(sizeof(__data_type))
#define ex_malloc_u8(length) (uint8_t*)Malloc(length)
#define ex_malloc_s8(length) (int8_t*)Malloc(length)

#define ex_realloc(__data_type, __old_ptr, __data_cnt)  \
            (__data_type*)realloc((__old_ptr), sizeof(__data_type) * (__data_cnt))

#define ex_memzero(__data_ptr, __data_size) memset((__data_ptr), 0, (__data_size))
#define ex_memzero_one(__data_ptr) memset((__data_ptr), 0, sizeof(*(__data_ptr)))

#ifdef __cplusplus
}
#endif
#endif  /* EX_MEMORY_H_201410021810 */
