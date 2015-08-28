/*
 * =============================================================================
 *      Filename    :   qj_err.c
 *      Description :
 *      Created     :   2013-11-23 21:11:37
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include "ex_errno.h"
#include "chash.h"
#include "clog.h"
#include "ex_memory.h"

typedef struct err_s {
    Status err;         /* 错误码 */
    const char *desc;   /* 错误描述 */
}err_t;

#if 0
static chash* g_errs = NULL;
#endif

/* ==========================================================================
 *        obj err ops
 * ======================================================================= */
void cobj_err_free(void *obj)
{
    err_t *err = obj;
    free(err);
}

void cobj_err_printf(FILE *file, const void *obj)
{
    const err_t *err = obj;
    fprintf(file, "%s", err->desc);
}

/* ==========================================================================
 *        err
 * ======================================================================= */
#if 0
static err_t* err_find(Status err_code)
{
    return chash_int_get(g_errs, err_code);
}

void ex_err_add(Status err_code, const char *desc)
{
    err_t *err = NULL;
    if(chash_int_exist(g_errs, err_code)) {
        log_warn("Already exist error code:%d", err_code);
    } else {
        err = ex_malloc_one(err_t);
        err->desc = desc;
        err->err  = err_code;

        chash_int_add(g_errs, err_code, err, sizeof(err_t));
    }
}

const char* ex_err_str(Status err_code)
{
    static char str[32] = {0};
    err_t *err = NULL;

    err = err_find(err_code);
    if(NULL != err && NULL != err->desc) return err->desc;
    else {
        sprintf(str, "Error Code:%d", err_code);
        return str;
    }
}

void  ex_err_init(void)
{
    cobj_ops_t ops_err;

    cobj_ops_init(&ops_err);
    ops_err.cb_free = cobj_err_free;
    ops_err.cb_printf = cobj_err_printf;
    cobj_ops_set_mode(&ops_err, COBJ_MODE_TAKE_OVER);

    g_errs = chash_int_new(&ops_err);

    ex_err_add(S_OK,  "OK");
    ex_err_add(S_ERR, "Error");

    /* chash_printf_test(stdout, g_errs); */
}

void  ex_err_release(void)
{
    chash_free(g_errs);
}
#endif
