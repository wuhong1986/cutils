/*
 * =============================================================================
 *      Filename    :   ex_assert.c
 *      Description :
 *      Created     :   2013-10-09 10:10:43
 *      Author      :    Wu Hong
 * =============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cstring.h"
#include "ccli.h"
#include "cthread.h"
#include "clog.h"
#include "cdefine.h"
#include "ex_file.h"
#include "ex_string.h"
#include "ex_memory.h"
#include<signal.h>
#ifdef __linux__
#define __USE_GNU
#include <dlfcn.h>
#ifndef QJ_BOARD_ANDROID
#include<execinfo.h>
#endif
#endif

static cstr *g_str_assert = NULL;
static cstr *g_str_signal = NULL;

static void write_bt_str_to_file(const char *file_name_pre, const cstr *str)
{
    FILE *pfile = NULL;
    char name[256] = {0};

    sprintf(name, "%s/%s.txt", log_get_dir(), file_name_pre);
    pfile = fopen(name, "a");
    if(NULL == pfile) {
        return;
    }

    ex_fwrite(pfile, cstr_body(str), cstr_len(str));

    ex_fclose(pfile);
}

#define BACKTRACE_MAX_SIZE 128
void get_backtrace(cstr *str)
{
#ifdef __linux__
#ifdef QJ_BOARD_ANDROID
#else
#if CPU_TYPE != CPU_TYPE_X86
#if 0

    int i                  = 0;
    int *pfp_register      = 0;
    int *pfp_next_register = 0;
    int stack_fp_array[BACKTRACE_MAX_SIZE] = {0};
    Dl_info symbol;

    __asm__(
        "mov %0, fp\n"
        : "=r"(pfp_register)
    );
    pfp_next_register = pfp_register;
    for ( i = 0; i < BACKTRACE_MAX_SIZE && pfp_next_register != 0; i++) {
        pfp_next_register = (int*)*(pfp_register-1);	//get ebp
        stack_fp_array[i] = *pfp_register; 	//get eip
        pfp_register = pfp_next_register;
    }

    for( i = 0; i < BACKTRACE_MAX_SIZE && stack_fp_array[i] != 0; i++) {
        if(0 == dladdr((void*) stack_fp_array[i], &symbol)) {
            cstr_append(str, "dladdr Failed!\n");
        } else {
            cstr_append(str, "#%-2d in %s at %s:0x%08x\n",i, symbol.dli_sname, symbol.dli_fname, stack_fp_array[i]);
        }
    }
#else
#endif
    void *array[BACKTRACE_MAX_SIZE];
    size_t size = 0;
    char **strings = NULL;
    size_t i = 0;

    size = backtrace(array, BACKTRACE_MAX_SIZE);
    strings = backtrace_symbols(array, size);

    if(NULL != strings){
        for (i = 0; i < size && strings != NULL; i++) {
            cstr_append(str, "==%02d== %s \n", (int)i, strings[i]);
        }

        ex_free(strings);
    } else {
        cstr_append(str, "get backtrace_symbols failed!");
    }
#endif
#endif
#else
    cstr_append(str, "Only support linux platform!");
#endif
}

static void singal_handler(int signum)
{
    if(NULL != g_str_signal){
        cstr_free(g_str_signal);
    }

    g_str_signal = cstr_new();

    cstr_append(g_str_signal, "*********************************************************\n");
    cstr_append(g_str_signal, "*\tCatch Signal:%d, \n", signum);
    cstr_append(g_str_signal, "*\tTime: %s\n", ex_strtime_t_now());
    cstr_append(g_str_signal, "*\tName: %s\n", thread_current_name());
    cstr_append(g_str_signal, "*********************************************************\n");
    signal(signum, SIG_DFL);
    get_backtrace(g_str_signal);
    write_bt_str_to_file("signal", g_str_signal);
    fprintf(stderr, "%s", cstr_body(g_str_signal));
    exit(1);
}

void backtrace_regist(int signum)
{
    signal(signum, singal_handler);
}

void ex_assert_do(const char *condition, const char *file, int line)
{
    if(NULL != g_str_assert){
        cstr_free(g_str_assert);
    }

    g_str_assert = cstr_new();

    cstr_append(g_str_assert, "*********************************************************\n");
    cstr_append(g_str_assert, "*              ASSERT INFO                              *\n");
    cstr_append(g_str_assert, "*\tcondition: %s\n", condition);
    cstr_append(g_str_assert, "*\t     File: %s\n", file);
    cstr_append(g_str_assert, "*\t     Line: %d\n", line);
    cstr_append(g_str_assert, "*\t     Time: %s\n", ex_strtime_t_now());
    cstr_append(g_str_assert, "*\t     Name: %s\n", thread_current_name());
    cstr_append(g_str_assert, "*********************************************************\n");

    get_backtrace(g_str_assert);
    fprintf(stderr, "%s", cstr_body(g_str_assert));
    write_bt_str_to_file("assert", g_str_assert);
#ifdef __linux__

    sigset_t supend_mask;
    sigemptyset(&supend_mask);
    sigaddset(&supend_mask, SIGUSR2);
    sigsuspend(&supend_mask);
#endif
}

#if 0
static void cli_assert_fun(const cli_arg_t *arg, cstr *cli_str)
{
    UNUSED(arg);
#ifdef __linux__
    cstr_append(cli_str, "------------- Last Assert Information -----------\n");
    if(NULL != g_str_assert){
        cstr_append(cli_str, "%s\n", cstr_body(g_str_assert));
    } else {
        cstr_append(cli_str, "No assert infomation!\n");
    }


    cstr_append(cli_str, "\n----- Last Signal Assert Information ----\n");
    if(NULL != g_str_signal){
        cstr_append(cli_str, "%s\n", cstr_body(g_str_signal));
    } else {
        cstr_append(cli_str, "No signal infomation!\n");
    }
#else
    cstr_append(cli_str, "Only support linux platform\n");
#endif
}
#endif

void ex_assert_init_cli(void)
{
#if 0
    cli_regist("assert" , cli_assert_fun ,
              "display last assert infomation." , "assert<CR>");
#endif

}
