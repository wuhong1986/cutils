/*
 * =============================================================================
 *      Filename    :   qj_log.c
 *      Description :
 *      Created     :   2013-10-09 10:10:49
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "cdefine.h"
#include "clist.h"
#include "cstring.h"
#include "cthread.h"
#include "csem.h"
#include "clog.h"
#include "ccli.h"
#include "cdefine.h"
#include "ex_file.h"
#include "ex_memory.h"
#include "ex_time.h"
#include "cobj_str.h"

static csem* g_sem_log    = NULL; /*  控制日志输出的信号量 */
static log_out_t *g_log_out_std  = NULL; /*  系统标准输出 */
static log_out_t *g_log_out_file = NULL; /*  输出到文件 */
static log_level_t g_min_level = LOG_LEVEL_DEBUG;   /* 最低输出级别 */
static bool g_items_enable[LOG_ITEM_CNT];   /* 输出项是否使能 */

static log_out_t g_list_outs[8];
static int g_list_outs_size = sizeof(g_list_outs) / sizeof(log_out_t);
static int g_outs_len = 0;
static clist *g_list_record = NULL;

static char  g_log_dir[MAX_LEN_PATH] = "./log.d/";
static FILE* g_pfile   = NULL; /*  保存日志的文件 */
static char  g_file_path[MAX_LEN_PATH] = {0};

static uint32_t g_cnt_record_max = 256;  /* 最多记录的历史记录个数 */
static callback_log_gps_time g_cb_gps = NULL;

void log_set_gps_time_callback(callback_log_gps_time cb)
{
    g_cb_gps = cb;
}

static FILE*  get_log_file(void)
{
    struct tm *tm_now = NULL;
    char path[MAX_LEN_PATH] = {0};

    ex_mkdir(g_log_dir);

    /* 生成文件名 根据日期命名 */
    tm_now = ex_time_now_tm();
    snprintf(path, sizeof(path), "%s/%d-%d-%d.log", g_log_dir,
                                                    tm_now->tm_year + 1900,
                                                    tm_now->tm_mon + 1,
                                                    tm_now->tm_mday);
    if(strcmp(path, g_file_path) != 0) {
        if(g_pfile) { fclose(g_pfile); g_pfile = NULL;}
        strcpy(g_file_path, path);
    }

    if(!g_pfile) {
        g_pfile = fopen(g_file_path, "a+");
        if(NULL == g_pfile){
            printf("open log file failed:%s, reason:%s\n", path, strerror(errno));
            assert(0);
        }
    }

    return g_pfile;
}

/**
 * @Brief  输出到标准输出STD的写函数
 *
 * @Param str 要写入的字符串
 */
static void log_write_to_std(const char *str)
{
    printf("%s", str);
}

/**
 * @Brief  输出到文件输出的写函数
 *
 * @Param str 要写入的字符串
 */
static void log_write_to_file(const char *str)
{
    g_pfile = get_log_file();

    if(NULL != g_pfile){
        fprintf(g_pfile, "%s", str);
    }
}

/**
 * @Brief  输出标准输出的FLUSH函数
 */
static void log_flush_to_std(void)
{
    fflush(stdout);
}

/**
 * @Brief  输出到文件的FLUSH函数
 */
static void log_flush_to_file(void)
{
    g_pfile = get_log_file();

    if(NULL != g_pfile) {
        fflush(g_pfile);
    }
}

/**
 * @Brief  对日志系统加锁，防止多线程／多进程不同步，导致输出不规则
 */
static void log_lock(void)
{
    csem_lock(g_sem_log);
}

/**
 * @Brief  写完毕后，对日志系统解锁，允许其他线程／进程写入
 */
static void log_unlock(void)
{
    csem_unlock(g_sem_log);
}

/**
 * @Brief  添加一个日志输出对象，比如在MFC中调试用的TRACE
 *
 * @Param name          该日志输出的名称
 * @Param write_fun     该输出的写入函数
 * @Param flush_fun     该输出的flush函数
 *
 * @Returns     该输出在日志系统中的引用
 */
log_out_t*  log_add_out(const char *name,
                        log_out_cb_write cb_write,
                        log_out_cb_flush cb_flush)
{
    log_out_t *log_out = NULL;

    if(g_outs_len >= g_list_outs_size) return NULL;

    log_out = &(g_list_outs[g_outs_len++]);

    log_out->name = name;
    log_out->cb_write = cb_write;
    log_out->cb_flush = cb_flush;

    return log_out;
}

/**
 * @Brief  设置保存日志文件的目录
 *
 * @Param dir   需要保存的目录, 为NULL则采用默认目录
 */
void  log_set_dir(const char *dir)
{
    if(NULL == dir) return;

    strcpy(g_log_dir, dir);
}

const char *log_get_dir(void)
{
    return g_log_dir;
}

/**
 * @Brief  输出字符串到每个日志输出系统, 日志信息的每个部分输出都是调用本函数
 *
 * @Param str          该条日志信息的内容
 */
static void log_output_str(const char *str)
{
    log_out_t  *out  = NULL;
    int i = 0;

    for(i = 0; i < g_outs_len; ++i) {
        out = &(g_list_outs[i]);
        if(out->cb_write) {
            out->cb_write(str);
        }
    }
}

static const char *get_level_str(log_level_t level)
{
    const char *log_str = NULL;

    switch(level){
        case LOG_LEVEL_DEBUG: log_str = "[D]"; break;
        case LOG_LEVEL_INFO:  log_str = "[I]"; break;
        case LOG_LEVEL_WARN:  log_str = "[W]"; break;
        case LOG_LEVEL_ERROR: log_str = "[E]"; break;
        default: log_str = "UNKNOWN"; break;
    }

    return log_str;
}

static void get_time_str(time_t time, int ms, cstr *buf)
{
    struct tm *tm_time = NULL;

    tm_time = localtime(&time);

    cstr_append(buf, "[%04d-%02d-%02d %02d:%02d:%02d.%03d]",
            tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
            tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, ms);
}

static void get_gps_time_str(time_t time, cstr *buf)
{
    struct tm *tm_time = NULL;

    tm_time = localtime(&time);

    cstr_append(buf, "[GPS:%04d-%02d-%02d %02d:%02d:%02d]",
            tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
            tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
}

/**
 * @Brief  输出日志信息的尾部，并刷新缓冲区，如C++中的endl
 *
 * @Param level 日志级别
 */
static void log_output_endl()
{
    log_out_t  *out  = NULL;
    int i = 0;

#ifdef __linux__
    log_output_str("\n");
#else
    log_output_str("\r\n");
#endif

    for(i = 0; i < g_outs_len; ++i) {
        out = &(g_list_outs[i]);
        if(out->cb_flush) {
            out->cb_flush();
        }
    }
}

/**
 * @Brief  日志系统的统一接口，该接口给logXXX调用
 *
 * @Param file  文件名
 * @Param line  行号
 * @Param func  函数名
 * @Param level 日志级别
 * @Param fmt   格式化字符串
 * @Param ...   参数
 *
 */
void log_printf(const char *file, int line,
                const char *func, log_level_t level,
                const char *fmt, ...)
{
    time_t  time_gps = 0;
    time_t  time_sec = 0;
    int     time_ms  = 0;
    va_list ap;
    cstr *str_body = NULL;
    cobj_str *obj_record = NULL;

    if(level >= LOG_LEVEL_OFF || level < g_min_level) return;

    str_body = cstr_new();

    log_lock();

    if(g_items_enable[LOG_ITEM_LEVEL]) {
        cstr_append(str_body, "%s", get_level_str(level));
    }

    if(g_items_enable[LOG_ITEM_FILE]) {
        cstr_append(str_body, "[%s:%d]", file, line);
    }

    if(g_items_enable[LOG_ITEM_FUNC]) {
        cstr_append(str_body, "[%s]", func);
    }

    if(g_items_enable[LOG_ITEM_DATE]) {
        time_sec = time(NULL);
        time_ms  = ex_time_now_ms();
        get_time_str(time_sec, time_ms, str_body);
    }

    if(g_items_enable[LOG_ITEM_GPS] && g_cb_gps){
        time_gps = g_cb_gps();
        get_gps_time_str(time_gps, str_body);
    }

    if(g_items_enable[LOG_ITEM_PIT]) {
        cstr_append(str_body, "[%02d]", thread_current_td());
    }


    /*  格式化传递参数 */
    va_start(ap, fmt);
    cstr_printf_va(str_body, fmt, ap);     /* 传递变长参数 */
    va_end(ap);

    /*  输出内容 */
    log_output_str(cstr_body(str_body));

    /*  输出换行符，fflush */
    log_output_endl();

    log_unlock();

    /* 添加到历史记录中 */
    clist_lock(g_list_record);

    while(clist_size(g_list_record) >= g_cnt_record_max){ /* 超过最大数，删除掉 */
        clist_remove_first(g_list_record);
    }

    obj_record = cobj_str_new(cstr_body(str_body));
    clist_append(g_list_record, obj_record);

    cstr_free(str_body);
    clist_unlock(g_list_record);
}

/**
 * @Brief  设置对应日志输出级别
 *
 * @Param level 输出的最低级别
 */
void log_set_level(log_level_t level)
{
    g_min_level = level;
}

/**
 * @Brief  设置相应级别输出时包含的内容
 *
 * @Param item 要设置的项
 *
 * @Param enable  是否使能
 *                true: 使能
 *                false: 不使能
 */
void  log_item_enable(log_item_t item, bool enable)
{
    g_items_enable[item] = enable;
}

/*
 * 获取日志标准输出
 */
log_out_t *log_out_std(void)
{
    return g_log_out_std;
}

/*
 * 获取日志文件输出
 */
log_out_t *log_out_file(void)
{
    return g_log_out_file;
}

static void log_cli_fun(cli_cmd_t *cmd)
{
    clist_node   *node   = NULL;
    void *record = NULL;

    clist_lock(g_list_record);

    clist_foreach_val(g_list_record, node, record){
        cli_output(cmd, "%s\n", cobj_str_val((cobj_str*)record));
    }

    clist_unlock(g_list_record);
}

/**
 * @Brief  初始化日志系统
 */
void log_init(void)
{
    g_list_record = clist_new();

    g_sem_log    = cmutex_new();

    g_items_enable[LOG_ITEM_LEVEL] = true;
    g_items_enable[LOG_ITEM_DATE] = true;
    g_items_enable[LOG_ITEM_PIT] = true;

    /*  添加系统标准输出和文件输出 */
    g_log_out_std  = log_add_out("std", log_write_to_std, log_flush_to_std);
    g_log_out_file = log_add_out("file", log_write_to_file, log_flush_to_file);

    cli_regist("log", log_cli_fun);
    cli_set_brief("log", "Show log infomation.");
    cli_add_option("log", "-l", "--level", "0~4, debug is 0, fatal is 4.", NULL);
}

void log_release(void)
{
    ex_fclose(g_pfile);

    clist_free(g_list_record);

    csem_free(g_sem_log);
}

