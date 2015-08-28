/* {{{
 * =============================================================================
 *      Filename    :   ex_memory.c
 *      Description :
 *      Created     :   2014-10-02 18:10:20
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "ex_memory.h"

void *Malloc(size_t size)
{
    void *ptr = malloc(size);
    if(NULL == ptr){
        assert(0);
    }

    memset(ptr, 0, size);

    return ptr;
}
