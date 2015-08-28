#ifndef CDEFINE_H_201410031310
#define CDEFINE_H_201410031310
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   cdefine.h
 *      Description :
 *      Created     :   2014-10-03 13:10:52
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

/* 解决QT 提示未定义变量告警 */
#define UNUSED(x)   (void)x

#define CPU_TYPE_X86      1
#define CPU_TYPE_ARM_9200 2
#define CPU_TYPE_ARM_9G45 3

#define U32_INVALID     (0xFFFFFFFE)
#define U16_INVALID     (0xFFFE)
#define U8_INVALID      (0xFE)

#define U32_NULL        (0xFFFFFFFD)
#define U16_NULL        (0xFFFD)
#define U8_NULL         (0xFD)

#define OUT
#define IN

#define SIZE_1KB    (1024)
#define SIZE_1MB    (SIZE_1KB * SIZE_1KB)
#define SIZE_10MB   (SIZE_1MB * 10)

#define MAX_LEN_PATH (256)

#ifdef __linux__
#define ATTRIBUTE_PACKED __attribute__((packed))
#else
#define ATTRIBUTE_PACKED
#endif

#ifdef __cplusplus
}
#endif
#endif  /* CDEFINE_H_201410031310 */
