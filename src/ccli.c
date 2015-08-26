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
#include "ex_file.h"
#include "ex_string.h"
#include "ccli.h"
#include "cobj_str.h"

cobj_ops_t cobj_ops_cli = {
    .name = "cli",
    .obj_size = sizeof(cli_t),
};

void cli_opt_destroy(void *obj)
{
    cli_opt_t *opt = (cli_opt_t*)obj;

    free(opt->argname);
    free(opt->large);
}

cobj_ops_t cobj_ops_cli_opt = {
    .name = "cli_opt",
    .obj_size = sizeof(cli_opt_t),
    .cb_destructor = cli_opt_destroy,
};

static bool  g_is_quit = false;
static chash *g_clis   = NULL;
static cstr  *g_str_welcome = NULL; /* 欢迎信息 */
static cstr  *g_str_prompt  = NULL;

bool cli_is_quit(void)
{
    return g_is_quit;
}

#if 0
/* ==========================================================================
 *        cli argument interface
 * ======================================================================= */
static cli_arg_t *cli_arg_new(void)
{
    cli_arg_t *arg = (cli_arg_t*)malloc(sizeof(cli_arg_t));

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
    clist_append(arg->argv, obj);
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
#endif

void cli_cmd_init(cli_cmd_t *cmd)
{
    cmd->cli         = NULL;
    cmd->is_finished = false;
    cmd->is_error    = false;
    cmd->opts        = chash_new();
    cmd->args        = cvector_new();
    cmd->output      = cstr_new();
}

void cli_cmd_release(cli_cmd_t *cmd)
{
    cmd->cli         = NULL;
    cmd->is_finished = false;
    cmd->is_error    = false;
    chash_free(cmd->opts);
    cvector_free(cmd->args);
    cstr_free(cmd->output);
}

void cli_cmd_clear(cli_cmd_t *cmd)
{
    cmd->cli         = NULL;
    cmd->is_finished = false;
    cmd->is_error    = false;
    chash_clear(cmd->opts);
    cvector_clear(cmd->args);
    cstr_clear(cmd->output);
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
static void cli_process(cli_cmd_t *cmd)
{
    const cli_t *cli = cmd->cli;

    if(cli && cli->cb) {
        cli->cb(cmd);
    } else if(!cli) {
        cstr_append(cmd->output, "No such command!\n");
    } else {
        cstr_append(cmd->output, "Undefine command callback!\n");
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
    cli_cmd_t cmd;

    cli_cmd_init(&cmd);

    while(!cli_is_quit()){
        printf("%s", cstr_body(g_str_prompt));

        memset(cli_input, 0, sizeof(cli_input));
        fgets(cli_input, sizeof(cli_input), stdin);

        cli_parse(&cmd, cli_input);
        printf("%s", cstr_body(cmd.output));
        cli_cmd_clear(&cmd);
	}
    cli_cmd_release(&cmd);
}

/*
 * Normalize the argument vector by exploding
 * multiple options (if any). For example
 * "foo -abc --scm git" -> "foo -a -b -c --scm git"
 */

static char ** normalize_args(int *argc, char **argv)
{
  int size = 0;
  int alloc = *argc + 1;
  char **nargv = malloc(alloc * sizeof(char *));
  int i, j;

  for (i = 0; argv[i]; ++i) {
    const char *arg = argv[i];
    size_t len = strlen(arg);

    // short flag
    if (len > 2 && '-' == arg[0] && !strchr(arg + 1, '-')) {
      alloc += len - 2;
      nargv = realloc(nargv, alloc * sizeof(char *));
      for (j = 1; j < len; ++j) {
        nargv[size] = malloc(3);
        sprintf(nargv[size], "-%c", arg[j]);
        size++;
      }
      continue;
    }

    // regular arg
    nargv[size] = malloc(len + 1);
    strcpy(nargv[size], arg);
    size++;
  }

  nargv[size] = NULL;
  *argc = size;
  return nargv;
}

static void command_parse_args(cli_cmd_t *self, int argc, char **argv) {
    const char *arg = NULL;
    const char *val = NULL;;
    cli_t *cli = self->cli;
    int literal = 0;
    int i, j;

  for (i = 1; i < argc; ++i) {
    arg = argv[i];
    val = NULL;
    /* printf("*** argv[%d]:%s\n", i, argv[i]); */
    for (j = 0; j < cvector_size(cli->options); ++j) {
      cli_opt_t *option = (cli_opt_t*)cvector_at(cli->options, j);

      // match flag
      if (!strcmp(arg, option->small) || !strcmp(arg, option->large)) {
        // required
        if (option->required_arg) {
            val = (i < argc - 1) ? argv[++i] : NULL;
          if (!val || '-' == val[0]) {
                fprintf(stderr, "%s %s argument required\n", option->large, option->argname);
                self->is_error = true;
                return;
          }
        }

        // optional
        if (option->optional_arg && i < argc - 1) {
          if (argv[i + 1] && '-' != argv[i + 1][0]) {
              val = argv[++i];
          }
        }

        if(option->cb) {
            option->cb(self);
            if(self->is_finished || self->is_error) {
                return;
            }
        }

        chash_str_str_set(self->opts, arg, val);
        /* chash_printf(self->opts, stdout); */

        goto match;
      }
    }

    // --
    if ('-' == arg[0] && '-' == arg[1] && 0 == arg[2]) {
      literal = 1;
      goto match;
    }

    // unrecognized
    if ('-' == arg[0] && !literal) {
        fprintf(stderr, "unrecognized flag %s\n", arg);
        self->is_error = true;
        return;
    }

    cvector_append(self->args, cobj_str_new(arg));
    /* cvector_print(self->args); */
    match:;
  }
}

/*
 * Parse `argv` (public).
 */

void command_parse(cli_cmd_t *self, int argc, char **argv)
{
    const char *name = argv[0];
    char **nargv = NULL;
    int i = 0;

    self->cli  = (cli_t*)chash_str_get(g_clis, name);
    nargv = normalize_args(&argc, argv);
    command_parse_args(self, argc, nargv);
    /* self->argv[self->argc] = NULL; */

    for(i = 0; i < argc; ++i) {
        free(nargv[i]);
    }
    free(nargv);
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
void cli_parse(cli_cmd_t *cmd, char *cmd_line)
{
    int argc = 0;
    char **argv = NULL;

    if(true == is_blank_line(cmd_line)) return;

    ex_str_trim(cmd_line);
    argc = ex_str_split(cmd_line, "\t\n\r ", &argv);

    command_parse(cmd, argc, argv);

    if(!cmd->is_finished && !cmd->is_error){
        cli_process(cmd);
    }
}

/*
 * Output command version.
 */

static void command_version(cli_cmd_t *self) {
  printf("Version: %s\n", self->cli->version);
  self->is_finished = true;
}

/*
 * Output command help.
 */

void command_help(cli_cmd_t *self) {
    cli_t *cli = self->cli;

    printf("\n");
    printf("  Usage: %s %s\n", cli->name, cli->usage);
    printf("\n");
    printf("  Options:\n");
    printf("\n");

    int i;
    for (i = 0; i < cvector_size(cli->options); ++i) {
        cli_opt_t *option = (cli_opt_t*)cvector_at(cli->options, i);
        printf("    %s, %-25s %s\n" , option->small ,
                                      option->large_with_arg ,
                                      option->description);
    }

    printf("\n");
    self->is_finished = true;
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
cli_t*  cli_regist(const char *name, cli_cmd_callback_t cb)
{
    cli_t *cli = (cli_t*)malloc(sizeof(cli_t));

    cobj_set_ops(cli, &cobj_ops_cli);
    cli->name    = name;
    cli->cb      = cb;
    cli->alias   = NULL;
    cli->desc    = "Undefined";
    cli->usage   = "[options]";
    cli->version = "1.0";
    cli->options = cvector_new();

    chash_str_set(g_clis, name, cli);

    cli_add_option(cli, "-V", "--version", "output program version", command_version);
    cli_add_option(cli, "-h", "--help", "output help information", command_help);

    return cli;
}

/*
 * Parse argname from `str`. For example
 * Take "--required <arg>" and populate `flag`
 * with "--required" and `arg` with "<arg>".
 */

static void parse_argname(const char *str, char *flag, char *arg) {
  int buffer = 0;
  size_t flagpos = 0;
  size_t argpos = 0;
  size_t len = strlen(str);
  size_t i;

  for (i = 0; i < len; ++i) {
    if (buffer || '[' == str[i] || '<' == str[i]) {
      buffer = 1;
      arg[argpos++] = str[i];
    } else {
      if (' ' == str[i]) continue;
      flag[flagpos++] = str[i];
    }
  }

  arg[argpos] = '\0';
  flag[flagpos] = '\0';
}

void cli_add_option(cli_t *cli,        const char *small,
                    const char *large, const char *desc,
                    cli_cmd_callback_t cb)
{

  cli_opt_t *option = (cli_opt_t*)malloc(sizeof(cli_opt_t));

  cobj_set_ops(option, &cobj_ops_cli_opt);

  option->cb    = cb;
  option->small = small;
  option->description = desc;
  option->required_arg = option->optional_arg = 0;
  option->large_with_arg = large;
  option->argname = malloc(strlen(large) + 1);
  option->large = malloc(strlen(large) + 1);
  parse_argname(large, option->large, option->argname);
  if ('[' == option->argname[0]) option->optional_arg = 1;
  if ('<' == option->argname[0]) option->required_arg = 1;

  cvector_append(cli->options, option);
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

#if 0
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
    strTemp = (char*)malloc(file_size);

    /*  读取shell命令输出内容，并拷贝到缓存 */
    pfile = fopen(temp_file, "rb");

    read_cnt = ex_fread(pfile, strTemp, file_size - 1);
    strTemp[read_cnt] = '\0';
    cstr_append(cli_str, "%s", strTemp);

    free(strTemp);
    ex_fclose(pfile);

    /*  删除保存结果临时文件 */
    snprintf(cmd_line, sizeof(cmd_line), "rm -f %s", temp_file);
    ret_system = system(cmd_line);
    if(ret_system < 0){
        cstr_append(cli_str, "System cmd run failed:%s\n", strerror(errno));
    }
#else
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
    clist_iter iter = clist_begin(g_clis);
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
#endif

void  cli_init(void)
{
    g_clis        = chash_new();
    g_str_welcome = cstr_new();
    g_str_prompt  = cstr_new();

    cstr_append(g_str_prompt, "#> ");

#if 0
    cli_regist_help("help");
    cli_regist_help("?");
    cli_regist_quit("quit");
    cli_regist_quit("q");

    cli_regist("shell"  , cli_shell,
              "run linux shell command." , "shell cmd<CR>");
#endif
}

void  cli_release(void)
{
    cstr_free(g_str_prompt);
    cstr_free(g_str_welcome);
    chash_free(g_clis);
}
