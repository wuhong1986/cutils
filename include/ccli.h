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

typedef struct cli_arg_s cli_arg_t;
/**
 * @Brief  CLI命令的回调函数
 *
 * @Param argc  命令所带的参数个数，不包括命令本身
 * @Param argv  命令所带的参数数组
 * @Param result_buf 处理完该命令后的返回的打印信息
 * @Param buf_len    返回的信息长度
 */
typedef void (*callback_cli)(const cli_arg_t *arg, cstr *cli_str);

/**
 * @Brief  CLI命令结构体
 */
typedef struct cli_s
{
    COBJ_HEAD_VARS;

    bool is_used;   /* 是否已经使用 */
    bool is_help;   /* 是否是帮助命令 */
    const char  *cmd;       /* 命令名称 */
    const char  *brief;     /* 该命令的概要说明 */
    const char  *help;      /* 该命令的帮助信息，使用说明 */
    callback_cli cb;        /* 该命令的回调函数 */
}cli_t;

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
 * @Param cmd   命令名称，即用户输入的命令
 * @Param func  处理该命令的回调函数
 * @Param brief 命令的概要
 * @Param help  该命令的帮助信息
 */
void  cli_regist(const char *cmd,   callback_cli func,
                 const char *brief, const char *help);

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
void cli_parse(const char *cmd_line, cstr *cli_str);

/**
 * @Brief  是否退出程序
 *
 * @Returns
 */
bool cli_is_quit(void);

#ifdef __cplusplus
}
#endif
#endif  /* CLI_H_201303060803 */
