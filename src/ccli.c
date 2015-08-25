/*
 * =============================================================================
 *      Filename    :   cli.c
 *      Description :
 *      Created     :   2013-03-06 08:03:49
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "clist.h"
#include "cdefine.h"
#include "ex_file.h"
#include "ex_memory.h"
#include "ccli.h"
#include "cobj_str.h"

cobj_ops_t cobj_ops_cli = {
    "cli",
    sizeof(cli_t),
    false,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    OBJ_CB_MEMSIZE_NULL,
};

struct cli_arg_s
{
    clist *argv;
};

static bool  g_is_quit = false;
static clist *g_list   = NULL;
static cstr  *g_str_welcome = NULL; /* 欢迎信息 */
static cstr  *g_str_prompt  = NULL;

bool cli_is_quit(void)
{
    return g_is_quit;
}

/* ==========================================================================
 *        cli argument interface
 * ======================================================================= */
static cli_arg_t *cli_arg_new(void)
{
    cli_arg_t *arg = ex_malloc_one(cli_arg_t);

    arg->argv = clist_new();

    return arg;
}

static void cli_arg_free(cli_arg_t *arg)
{
    clist_free(arg->argv);
    free(arg);
}

uint32_t cli_arg_cnt(const cli_arg_t *arg)
{
    return clist_size(arg->argv);
}

static void cli_arg_add(cli_arg_t *arg, const char *str)
{
    cobj_str *obj = cobj_str_new(str);
    clist_append(arg->argv, CAST_TO_COBJ(obj));
}

static const char* cli_arg_get(const cli_arg_t *arg, uint32_t idx)
{
    cobj_str *obj = (cobj_str*)clist_at_obj(arg->argv, idx);

    if(obj != NULL) {
        return cobj_str_val(obj);
    } else {
        return NULL;
    }
}

const char* cli_arg_get_str(const cli_arg_t *arg, uint32_t idx, const char* def)
{
    const char *val = cli_arg_get(arg, idx);

    return val == NULL ? def : val;
}

int cli_arg_get_int(const cli_arg_t *arg, uint32_t idx, int def)
{
    const char *val = cli_arg_get(arg, idx);

    return val == NULL ? def : atoi(val);
}

uint32_t cli_arg_get_hex(const cli_arg_t *arg, uint32_t idx, uint32_t def)
{
    const char *val = cli_arg_get(arg, idx);
    return val == NULL ? def : strtoul(val, 0, 16);
}

float cli_arg_get_float(const cli_arg_t *arg, uint32_t idx, float def)
{
    float value = def;
    const char *val = cli_arg_get(arg, idx);

    if(val != NULL){
        sscanf(val, "%f", &value);
        return value;
    } else {
        return def;
    }
}

/**
 * @Brief  从参数列表中删除指定序号的参数
 *
 * @Param arg   参数列表
 * @Param idx   参数序号
 */
void cli_arg_remove(cli_arg_t *arg, uint32_t idx)
{
    clist_remove_at(arg->argv, idx);
}

void cli_arg_remove_first(cli_arg_t *arg)
{
    cli_arg_remove(arg, 0);
}

/**
 * @Brief  根据命令名称cmd查找保存在clis中对应的CLI命令
 *
 * @Param cmd  要查找的命令名称
 *
 * @Returns     NULL: 没有找到
 *              否则返回对应的CLI
 */
static const cli_t* find_cli(const char *cmd)
{
    clist_iter iter = clist_begin(g_list);
    cli_t      *cli     = NULL;

    clist_iter_foreach_obj(&iter, cli){
        if(strcmp(cli->cmd, cmd) == 0){ /*  find it */
            return cli;
        }
    }

    return NULL;
}

/**
 * @Brief  根据命令名称，参数查找处理函数处理命令
 *
 * @Param cmd   命令名称
 * @Param argc  参数个数
 * @Param argv  参数数组
 * @Param result_buf 返回信息
 * @Param buf_len    返回信息长度
 *
 * @Returns
 */
static void cli_process(const char *cmd, const cli_arg_t *arg, cstr *cli_str)
{
    const cli_t   *cli = NULL;

    /*  找到对应的命令处理函数 */
    cli = find_cli(cmd);
    if(NULL == cli || NULL == cli->cb)  {
        cstr_append(cli_str, "No such command!\n");
    } else {
        cli->cb(arg, cli_str);
    }
}

/**
 * @Brief  判断字符是否是参数分割符
 *
 * @Param ch
 *
 * @Returns
 */
bool is_arg_split(char ch)
{
    if(' ' == ch || '\t' == ch || '\n' == ch || '\r' == ch) return true;
    else return false;
}

/**
 * @Brief  判断当前输入行是否是空行
 *
 * @Param cmd_line
 *
 * @Returns
 */
bool is_blank_line(const char *cmd_line)
{
    uint32_t i = 0;

    for (i = 0; i < strlen(cmd_line); i++) {
        /*printf("char %c\n", cmd_line[i]);*/
        switch(cmd_line[i]){
            case '\t':
            case '\n':
            case ' ':
            case '\r':
                continue;
                break;
            default:
                return false;
        }
    }

    return true;
}

cstr* cli_get_welcome_str(void)
{
    return g_str_welcome;
}

cstr* cli_get_prompt_str(void)
{
    return g_str_prompt;
}

void cli_welcome(void)
{
    printf("%s", cstr_body(g_str_welcome));
}

void cli_loop(void)
{
    char  cli_input[256] = {0};
    cstr *cli_str = NULL;

    cli_str = cstr_new();
    while(!cli_is_quit()){
        printf("%s", cstr_body(g_str_prompt));

        ex_memzero(cli_input, sizeof(cli_input));
        fgets(cli_input, sizeof(cli_input), stdin);

        cli_parse(cli_input, cli_str);
        printf("%s", cstr_body(cli_str));
        cstr_clear(cli_str);
	}
    cstr_free(cli_str);
}

/**
 * @Brief  接收命令行cmd_line, 并处理
 *
 * @Param cmd_line   命令行，即用户输入的一行命令
 * @Param result_buf 处理该命令的返回内容,
 *                   该空间会在各个命令处理函数中分配，
 *                   调用程序在使用完result_buf后需要显示调用free释放内存
 * @Param buf_len    返回内容的长度
 *
 * @Returns   STATUS_OK 为处理该命令正常
 *            其他值为对应的错误状态码
 */
void cli_parse(const char *cmd_line, cstr *cli_str)
{
    char  *cmd  = NULL;
    char  *str  = NULL;
    cli_arg_t *arg = NULL;
    uint32_t cmd_len       = 0;
    uint32_t cmd_line_len  = 0;
    uint32_t arg_idx       = 0;
    uint32_t arg_len       = 0;
    uint32_t arg_pos_start = 0;
    uint32_t arg_pos_end   = 0;
    uint32_t i             = 0;
    uint32_t j             = 0;

    if(true == is_blank_line(cmd_line)) return;

    cmd_line_len = strlen(cmd_line);
    for (i = 0; i < cmd_line_len; i++) {
        if(true == is_arg_split(cmd_line[i])) {
            cmd_len = i;
            break;
        }
    }

    cmd = ex_malloc(char, cmd_len + 1);     /*  +1 是给'\0'分配的 */
    memcpy(cmd, cmd_line, cmd_len);
    cmd[cmd_len] = '\0';

    arg  = cli_arg_new();

    /*  再解析一次，这次获取具体的参数值 */
    for (i = cmd_len; i < cmd_line_len; ) {
        /*  当前的字符是分割字符，但是还需要判断下一个字符，比如2个空格的情况 */
        if( true == is_arg_split(cmd_line[i])
        && (i + 1 < cmd_line_len)
        && false == is_arg_split(cmd_line[i + 1])) { /* 下一个字符不是分割字符，
                                                        则参数+1 */
            /*  已经进入参数的起始位置，获取参数的长度 */
            arg_pos_start = i + 1;
            for (j = arg_pos_start + 1; j < cmd_line_len; j++) {
                if(true == is_arg_split(cmd_line[j])){ /*  参数结束了 */
                    arg_pos_end = j;
                }
                else if(j == cmd_line_len - 1){ /*  已经是最后一个字符类，也认为是结束了 */
                    arg_pos_end = cmd_line_len;
                }

                if(arg_pos_end > arg_pos_start) { /*  找到参数了，计算长度，并跳出循环, 查找下一个参数 */
                    arg_len = arg_pos_end - arg_pos_start;
                    str     = ex_malloc(char, arg_len + 1);

                    /*LoggerDebug("arg_len:%d, start:%d, end:%d", arg_len, arg_pos_start, arg_pos_end);*/
                    /*  拷贝参数 */
                    memcpy(str, &(cmd_line[arg_pos_start]), arg_len);
                    str[arg_len] = '\0';

                    cli_arg_add(arg, str);

                    ++arg_idx;
                    i = arg_pos_end;
                    break;
                }
            }
        } else {
            ++i;
        }
    }

    cli_process(cmd, arg, cli_str);

    /*  处理完命令后， 释放之前分配的空间 */
    ex_free(cmd);
    cli_arg_free(arg);
}

/**
 * @Brief  注册CLI命令
 *
 * @Param cmd   命令名称，即用户输入的命令
 * @Param func  处理该命令的回调函数
 * @Param brief 命令的概要
 * @Param help  该命令的帮助信息
 * @Param is_help 是否是帮助命令
 *
 * @Returns
 */
void  cli_regist_cmd(const char *cmd, callback_cli cb,
                 const char *brief, const char *help, bool is_help)
{
    cli_t *cli = (cli_t*)malloc(sizeof(cli_t));

    cli->ops     = &cobj_ops_cli;
    cli->is_help = is_help;
    cli->cmd     = cmd;
    cli->cb      = cb;
    if(NULL != brief) cli->brief = brief;
    else    cli->brief = "No brief.";

    if(NULL != help)  cli->help = help;
    else    cli->help = "No help.";

    clist_append(g_list, CAST_TO_COBJ(cli));
}

/**
 * @Brief  注册普通CLI命令
 *
 * @Param cmd   命令名称，即用户输入的命令
 * @Param func  处理该命令的回调函数
 * @Param brief 命令的概要
 * @Param help  该命令的帮助信息
 *
 * @Returns
 */
void  cli_regist(const char *cmd, callback_cli func,
                 const char *brief, const char *help)
{
    cli_regist_cmd(cmd, func, brief, help, false);
}

#if 0
static int    cli_remote_at_cmd(uint32_t argc, char **argv, cstr *cli_str)
{
    const char *ni     = NULL;
    const char *cmd    = NULL;
    CommAddr_S  *pAddr = NULL;

    if(argc < 2){
        cstr_append(cli_str, "Please input at least 2 param!");
    return S_OK;
    }

    ni  = argv[0];
    cmd = argv[1];

    pAddr = CommAddrFindByNI(ni);
    ZigbeeSendRemoteATCmdNoParam(pAddr, cmd);

    return S_OK;
}
#endif

static void cli_shell(const cli_arg_t *arg, cstr *cli_str)
{
#ifdef __linux__
    char     cmd_line[512];
    uint32_t offset     = 0;
    uint32_t argc       = cli_arg_cnt(arg);
    uint32_t i          = 0;
    FILE     *pfile     = NULL;
    int32_t  read_cnt   = 0;
    uint32_t file_size  = 0;
    char     *strTemp   = NULL;
    int      ret_system = 0;
    const char *temp_file = ".temp_cmd_shell_output";

    if(argc <= 0){
        cstr_append(cli_str, "Please input shell cmd!\n");
        return ;
    }

    memset(cmd_line, 0, sizeof(cmd_line));

    for (i = 0; i < argc; i++) {
        snprintf(&(cmd_line[offset]), sizeof(cmd_line),
                 "%s ", cli_arg_get_str(arg, i, ""));
        offset = strlen(cmd_line);
    }

    snprintf(&(cmd_line[offset]), sizeof(cmd_line), ">%s", temp_file);
    ret_system = system(cmd_line);
    if(ret_system < 0){
        cstr_append(cli_str, "System cmd run failed:%s\n", strerror(errno));
    }

    file_size = ex_file_size(temp_file) + 1;
    strTemp = ex_malloc(char, file_size);

    /*  读取shell命令输出内容，并拷贝到缓存 */
    pfile = fopen(temp_file, "rb");

    read_cnt = ex_fread(pfile, strTemp, file_size - 1);
    strTemp[read_cnt] = '\0';
    cstr_append(cli_str, "%s", strTemp);

    ex_free(strTemp);
    ex_fclose(pfile);

    /*  删除保存结果临时文件 */
    snprintf(cmd_line, sizeof(cmd_line), "rm -f %s", temp_file);
    ret_system = system(cmd_line);
    if(ret_system < 0){
        cstr_append(cli_str, "System cmd run failed:%s\n", strerror(errno));
    }
#else
    UNUSED(arg);
    cstr_append(cli_str, "Only Support Linux Platform\n");
#endif
}

/**
 * @Brief  CLI退出命令
 *
 * @Param argc
 * @Param argv
 * @Param result_buf
 * @Param buf_len
 *
 * @Returns
 */
void cli_quit_fun(const cli_arg_t *arg, cstr *cli_str)
{
    UNUSED(arg);

    cstr_append(cli_str, "Bye...\n");

    g_is_quit = true;
}

/**
 * @Brief  CLI帮助命令
 *
 * @Param argc
 * @Param argv
 * @Param result_buf
 * @Param buf_len
 *
 * @Returns
 */
void cli_help_fun(const cli_arg_t *arg, cstr *cli_str)
{
    clist_iter iter = clist_begin(g_list);
    const cli_t *cli  = NULL;
    uint32_t argc = cli_arg_cnt(arg);

    /*
     *  根据输入的参数判断是查询具体某个命令的帮助信息，还是特指某个命令
     *      1. 如果没有带参数，则输出所有命令的brief
     *      2. 如果带参数，则认为第一个参数是要查找帮助的命令，后面的忽略
     */
    if(argc > 0){ /*  查询特定命令 */
        cli = find_cli(cli_arg_get_str(arg, 0, "help"));
        if(NULL == cli) {
            cstr_append(cli_str, "No such command!\n");
        } else {
            /*
             * 返回帮助信息的格式：
             *      cmd\tbrief\n
             *      Usage:\thelp\n
             */
            cstr_append(cli_str, "Brief:\t%s\nUsage:\t%s\n", cli->brief, cli->help);
        }
    } else { /*  查询所有命令 */
        /*
         * 返回帮助信息的格式：
         *  cmd 占20个字节，因此命令最长占20个字节
         *   Cmd    brief
         *   Loop:
         *      cmd brief\n
         *   End Loop
         */
        cstr_append(cli_str, "%-20s%s\n", "Command", "Brief");
        cstr_append(cli_str, "---------------------------------------\n");

        clist_iter_foreach_obj(&iter, cli){
            if(false == cli->is_help) {
                cstr_append(cli_str, "%-20s%s\n", cli->cmd, cli->brief);
            }
        }
    }
}

/**
 * @Brief  注册帮助命令
 *
 * @Param cmd 帮助命令的名称
 *
 * @Returns
 */
static void cli_regist_help(const char *cmd)
{
    cli_regist_cmd(cmd, cli_help_fun, NULL, NULL, true);
}

/**
 * @Brief  注册退出命令
 *
 * @Param cmd
 *
 * @Returns
 */
static void cli_regist_quit(const char *cmd)
{
    cli_regist_cmd(cmd, cli_quit_fun, "quit program.", "quit <CR>", false);
}

void  cli_init(void)
{
    g_list        = clist_new();
    g_str_welcome = cstr_new();
    g_str_prompt  = cstr_new();

    cli_regist_help("help");
    cli_regist_help("?");
    cli_regist_quit("quit");
    cli_regist_quit("q");

    cli_regist("shell"  , cli_shell,
              "run linux shell command." , "shell cmd<CR>");
}

void  cli_release(void)
{
    cstr_free(g_str_prompt);
    cstr_free(g_str_welcome);
    clist_free(g_list);
}
