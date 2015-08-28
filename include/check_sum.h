#ifndef CHECK_SUM_H_201409100909
#define CHECK_SUM_H_201409100909
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   check_sum.h
 *      Description :
 *      Created     :   2014-09-10 09:09:12
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <stdint.h>

uint8_t check_sum_u8(const void *data, uint32_t len);
uint8_t check_sum_u8_2(const void *data, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif  /* CHECK_SUM_H_201409100909 */
