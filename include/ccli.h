#ifndef QJ_CLI_CORE_H_201303060803
#define QJ_CLI_CORE_H_201303060803
#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 *      Filename    :   qj_cli_core.h
 *      Description :
 *      Created     :   2013-03-06 08:03:43
 *      Author      :    Wu Hong
 * =============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include "cstring.h"
#include "cobj.h"
#include "chash.h"
#include "cvector.h"

#define cli_output(cmd, fmt, args...) cstr_append((cmd)->output, fmt, ##args)

typedef struct cli_cmd cli_cmd_t;
/**
 * @Brief  CLI命令的回调函数
 *
 * @Param argc  命令所带的参数个数，不包括命令本身
 * @Param argv  命令所带的参数数组
 * @Param result_buf 处理完该命令后的返回的打印信息
 * @Param buf_len    返回的信息长度
 */
typedef void (*cli_cmd_callback_t)(cli_cmd_t *cmd);
typedef void (*cli_quit_callback_t)(void);

typedef struct {
    COBJ_HEAD_VARS;

    int optional_arg;
    int required_arg;
    char *argname;
    char *large;
    const char *small;
    const char *large_with_arg;
    const char *description;

    cli_cmd_callback_t cb;
} cli_opt_t;

/**
 * @Brief  CLI命令结构体
 */
typedef struct cli_s
{
    COBJ_HEAD_VARS;

    const char *name;
    const char *alias;
    const char *desc;
    const char *usage;
    const char *version;

    cvector *options;

    cli_cmd_callback_t cb;
}cli_t;

struct cli_cmd{
    cli_t   *cli;
    bool    is_finished;
    bool    is_error;

    chash   *opts;
    cvector *args;
    cstr    *output;
} ;

#if 0
/* ==========================================================================
 *        cli argument interface
 * ======================================================================= */
/**
 * @Brief  获取cli参数个数
 *
 * @Param arg
 *
 * @Returns
 */
uint32_t cli_arg_cnt(const cli_arg_t *arg);
/**
 * @Brief  获取cli 参数列表中的字符串参数
 *
 * @Param arg   cli 参数列表
 * @Param idx   需要获取的参数序列号
 * @Param def   默认值
 *
 * @Returns
 */
const char* cli_arg_get_str(const cli_arg_t *arg, uint32_t idx, const char* def);
/**
 * @Brief  获取cli 参数列表中的整形(int)参数
 *
 * @Param arg   cli 参数列表
 * @Param idx   需要获取的参数序列号
 * @Param def   默认值
 *
 * @Returns
 */
int cli_arg_get_int(const cli_arg_t *arg, uint32_t idx, int def);
/**
 * @Brief  获取cli 参数列表中的16进制参数
 *
 * @Param arg   cli 参数列表
 * @Param idx   需要获取的参数序列号
 * @Param def   默认值
 *
 * @Returns
 */
uint32_t cli_arg_get_hex(const cli_arg_t *arg, uint32_t idx, uint32_t def);
float cli_arg_get_float(const cli_arg_t *arg, uint32_t idx, float def);

/**
 * @Brief  从参数列表中删除指定序号的参数
 *
 * @Param arg   参数列表
 * @Param idx   参数序号
 */
void cli_arg_remove(cli_arg_t *arg, uint32_t idx);
void cli_arg_remove_first(cli_arg_t *arg);
#endif

/* ===============================================================================
 *          cli interfaces
 * =============================================================================== */

/**
 * @Brief  CLI命令初始化
 *
 */
void  cli_init(void);

/**
 * @Brief  释放cli模块所占用的资源
 */
void  cli_release(void);

/**
 * @Brief  注册普通CLI命令
 *
 * @Param name  命令名称，即用户输入的命令
 * @Param func  处理该命令的回调函数
 * @Param brief 命令的概要
 * @Param help  该命令的帮助信息
 */
cli_t* cli_regist(const char *name, cli_cmd_callback_t func);
cli_t* cli_regist_alias(const char *name, const char *alias);

void cli_add_option(cli_t *cli,        const char *small,
                    const char *large, const char *desc,
                    cli_cmd_callback_t cb);

cstr* cli_get_welcome_str(void);
cstr* cli_get_prompt_str(void);

void cli_welcome(void);
void cli_loop(void);
/**
 * @Brief  接收命令行cmd_line, 并处理
 *
 * @Param cmd_line   命令行，即用户输入的一行命令
 * @Param result_buf 处理该命令的返回内容,
 *                   该空间会在各个命令处理函数中分配，
 *                   调用程序在使用完result_buf后需要显示调用free释放内存
 * @Param buf_len    返回内容的长度
 */
void cli_parse(cli_cmd_t *cmd, char *cmd_line);

/**
 * @Brief  是否退出程序
 *
 * @Returns
 */
bool cli_is_quit(void);
bool cli_set_quit_cb(cli_quit_callback_t cb);

#ifdef __cplusplus
}
#endif
#endif  /* CLI_H_201303060803 */
