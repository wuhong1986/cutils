/* {{{
 * =============================================================================
 *      Filename    :   check_sum.c
 *      Description :
 *      Created     :   2014-09-10 09:09:46
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include "check_sum.h"

uint8_t check_sum_u8(const void *data, uint32_t len)
{
    uint32_t i    = 0;
    uint32_t nsum = 0;

	for(i = 0; i < len; i++) {
	    nsum += ((uint8_t*)data)[i];
	}

    return 0xFF - nsum % 256;
}

uint8_t check_sum_u8_2(const void *data, uint32_t len)
{
    uint32_t i    = 0;
    uint32_t nsum = 0;

	for(i = 0; i < len; i++) {
	    nsum += ((uint8_t*)data)[i];
	}

    return nsum % 256;
}
