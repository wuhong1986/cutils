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
#include "ex_time.h"
#include "ccli.h"
#include "cobj_str.h"

void cli_destroy(void *obj)
{
    cli_t *cli = (cli_t*)obj;

    cvector_free(cli->options);
}

cobj_ops_t cobj_ops_cli = {
    .name = "cli",
    .obj_size = sizeof(cli_t),
    .cb_destructor = cli_destroy,
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
static cli_quit_callback_t g_quit_cbs[8];
static int g_quit_cb_idx = 0;
static chash *g_clis   = NULL;
static cstr  *g_str_welcome = NULL; /* 欢迎信息 */
static cstr  *g_str_prompt  = NULL;
static char  *g_str_prompt_extra = NULL;
static chash *g_opts = NULL;

bool cli_is_quit(void)
{
    return g_is_quit;
}

bool cli_add_quit_cb(cli_quit_callback_t cb)
{
    g_quit_cbs[g_quit_cb_idx++] = cb;
}

/* ==========================================================================
 *        cli argument interface
 * ======================================================================= */
uint32_t cli_arg_cnt(const cli_cmd_t *cmd)
{
    return cvector_size(cmd->args);
}

static const char* cli_arg_get(const cli_cmd_t *cmd, uint32_t idx)
{
    cobj_str *obj = NULL;

    if(cvector_size(cmd->args) <= idx) {
        return NULL;
    }

    obj = (cobj_str*)cvector_at(cmd->args, idx);

    if(obj != NULL) {
        return cobj_str_val(obj);
    } else {
        return NULL;
    }
}

const char* cli_arg_get_str(const cli_cmd_t *cmd, uint32_t idx, const char* def)
{
    const char *val = cli_arg_get(cmd, idx);

    return val == NULL ? def : val;
}

int cli_arg_get_int(const cli_cmd_t *cmd, uint32_t idx, int def)
{
    const char *val = cli_arg_get(cmd, idx);

    return val == NULL ? def : atoi(val);
}

uint32_t cli_arg_get_hex(const cli_cmd_t *cmd, uint32_t idx, uint32_t def)
{
    const char *val = cli_arg_get(cmd, idx);
    return val == NULL ? def : strtoul(val, 0, 16);
}

float cli_arg_get_float(const cli_cmd_t *cmd, uint32_t idx, float def)
{
    float value = def;
    const char *val = cli_arg_get(cmd, idx);

    if(val != NULL){
        sscanf(val, "%f", &value);
        return value;
    } else {
        return def;
    }
}

bool cli_opt_exist(const cli_cmd_t *cmd, const char *opt)
{
    return chash_str_haskey(cmd->opts, opt);
}

static const char* cli_opt_get(const cli_cmd_t *cmd, const char *opt)
{
    const char* val = NULL;

    val = chash_str_str_get(cmd->opts, opt);
    if(val) { return val; }
    else { return chash_str_str_get(g_opts, opt); }
}

const char* cli_opt_get_str(const cli_cmd_t *cmd, const char *opt, const char* def)
{
    const char *val = cli_opt_get(cmd, opt);

    return val == NULL ? def : val;
}

int cli_opt_get_int(const cli_cmd_t *cmd, const char *opt, int def)
{
    const char *val = cli_opt_get(cmd, opt);

    return val == NULL ? def : atoi(val);
}

uint32_t cli_opt_get_hex(const cli_cmd_t *cmd, const char *opt, uint32_t def)
{
    const char *val = cli_opt_get(cmd, opt);
    return val == NULL ? def : strtoul(val, 0, 16);
}

float cli_opt_get_float(const cli_cmd_t *cmd, const char *opt, float def)
{
    float value = def;
    const char *val = cli_opt_get(cmd, opt);

    if(val != NULL){
        sscanf(val, "%f", &value);
        return value;
    } else {
        return def;
    }
}

void cli_set_default_opt(const char *opt, const char *val)
{
    chash_str_str_set(g_opts, opt, val);
}

void cli_cmd_init(cli_cmd_t *cmd)
{
    cmd->cli            = NULL;
    cmd->is_finished    = false;
    cmd->is_error       = false;
    cmd->error          = CLI_CMD_OK;
    cmd->status         = 0;
    cmd->opts           = chash_new();
    cmd->args           = cvector_new();
    cmd->output         = cstr_new();
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
    cmd->cli            = NULL;
    cmd->is_finished    = false;
    cmd->is_error       = false;
    cmd->error          = CLI_CMD_OK;
    cmd->status         = 0;
    chash_clear(cmd->opts);
    cvector_clear(cmd->args);
    cstr_clear(cmd->output);
}

void cli_cmd_to_json(const cli_cmd_t *cmd, cstr *json)
{
    cstr_append(json, "{\n");
    cstr_append(json, "\"head\": {\"");
    cstr_append(json, "error\": %d", cmd->error);
    cstr_append(json, ", \"status\": %d", cmd->status);
    cstr_append(json, "},\n");
    cstr_append(json, "\"body\": %s", cstr_body(cmd->output));
    cstr_append(json, "}\n");
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
        cmd->error = CLI_CMD_NO_SUCH_CMD;
        cstr_append(cmd->output, "No such command!\n");
    } else {
        cmd->error = CLI_CMD_NO_SUCH_CMD;
        cstr_append(cmd->output, "Undefine command callback!\n");
    }
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
        /*cli_output(self, "char %c\n", cmd_line[i]);*/
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

void cli_set_extra_prompt(const char *extra_prompt)
{
    free(g_str_prompt_extra);
    g_str_prompt_extra = ex_strdup(extra_prompt);
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

    printf("Type \"ls\" for list commands. Type \"<cmd> -h\" for help.\n");
    while(!cli_is_quit()){
        printf("%s", cstr_body(g_str_prompt));
        if(g_str_prompt_extra && strcmp(g_str_prompt_extra, "") != 0) {
            printf("-%s ", g_str_prompt_extra);
        }
        printf("> ");

        memset(cli_input, 0, sizeof(cli_input));
        fgets(cli_input, sizeof(cli_input), stdin);

        if(true == is_blank_line(cli_input)) continue;

        cli_parse(&cmd, cli_input);
#if 0
        printf("{\n");
        printf("\"head\": {\"");
        printf("error\": %d", cmd.error);
        printf(", \"status\": %d", cmd.status);
        printf("},\n");
        printf("\"body\": %s", cstr_body(cmd.output));
        printf("}\n");
#else
        printf("%s", cstr_body(cmd.output));
#endif
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

  for (i = 0; i < *argc && argv[i]; ++i) {
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
    /* cli_output(self, "*** argv[%d]:%s\n", i, argv[i]); */
    for (j = 0; j < cvector_size(cli->options); ++j) {
      cli_opt_t *option = (cli_opt_t*)cvector_at(cli->options, j);

      // match flag
      if (!cli->disable_option
       && (!strcmp(arg, option->small) || !strcmp(arg, option->large))) {
        // required
        if (option->required_arg) {
            val = (i < argc - 1) ? argv[++i] : NULL;
          if (!val || '-' == val[0]) {
                cli_output(self, "%s %s argument required\n", option->large, option->argname);
                self->is_error = true;
                self->error = CLI_CMD_OPT_REQUIRE_ARG;
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
    if(!cli->disable_option && '-' == arg[0] && '-' == arg[1] && 0 == arg[2]) {
      literal = 1;
      goto match;
    }

    // unrecognized
    if (!cli->disable_option && '-' == arg[0] && !literal) {
        cli_output(self, "unrecognized flag %s\n", arg);
        self->is_error = true;
        self->error = CLI_CMD_OPT_ERROR;
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
    /* find alias */
    if(self->cli && self->cli->alias) {
        self->cli = (cli_t*)chash_str_get(g_clis, self->cli->alias);
    }

    if(self->cli){

        nargv = normalize_args(&argc, argv);
        command_parse_args(self, argc, nargv);

        for(i = 0; i < argc; ++i) {
            free(nargv[i]);
        }
        free(nargv);
    }
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
    argc = ex_str_split_charset(cmd_line, "\t\n\r ", &argv);

    command_parse(cmd, argc, argv);

    if(!cmd->is_finished && !cmd->is_error){
        cli_process(cmd);
    }

    ex_str_split_free(argv,argc);
}

/*
 * Output command version.
 */

static void command_version(cli_cmd_t *self)
{
    cli_output(self, "Version: %s\n", self->cli->version);
    self->is_finished = true;
}

/*
 * Output command help.
 */

static void command_help(cli_cmd_t *self)
{
    int i = 0;
    cli_t *cli = self->cli;

    cli_output(self, "\n");
    cli_output(self, "  Brief: %s\n", cli->desc);
    cli_output(self, "\n");
    cli_output(self, "  Usage: %s %s\n", cli->name, cli->usage);
    cli_output(self, "\n");
    cli_output(self, "  Options:\n");
    cli_output(self, "\n");

    for (i = 0; i < cvector_size(cli->options); ++i) {
        cli_opt_t *option = (cli_opt_t*)cvector_at(cli->options, i);
        cli_output(self, "    %s, %-25s %s\n" , option->small ,
                                      option->large_with_arg ,
                                      option->description);
    }

    cli_output(self, "\n");
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
    cli->name           = name;
    cli->cb             = cb;
    cli->alias          = NULL;
    cli->disable_option = false;
    cli->desc           = "Undefined";
    cli->usage          = "[options]";
    cli->version        = "1.0";
    cli->options        = cvector_new();

    chash_str_set(g_clis, name, cli);

    cli_add_option(name, "-V", "--version", "output program version", command_version);
    cli_add_option(name, "-h", "--help", "output help information", command_help);

    return cli;
}

cli_t* cli_regist_alias(const char *name, const char *alias)
{
    cli_t *cli = (cli_t*)malloc(sizeof(cli_t));

    cobj_set_ops(cli, &cobj_ops_cli);
    cli->name    = name;
    cli->alias   = alias;
    cli->options = NULL;

    chash_str_set(g_clis, name, cli);

    return cli;
}

void cli_set_brief(const char *name, const char *brief)
{
    cli_t *cli = chash_str_get(g_clis, name);
    if(cli){
        cli->desc = brief;
    }
}

void cli_enable_option(const char *name, bool enable)
{
    cli_t *cli = chash_str_get(g_clis, name);
    if(cli){
        cli->disable_option = !enable;
    }
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

void cli_add_option(const char *name,  const char *small,
                    const char *large, const char *desc,
                    cli_cmd_callback_t cb)
{
    cli_t *cli = NULL;
    cli_opt_t *option = NULL;

    cli = chash_str_get(g_clis, name);
    if(!cli) {
        printf("No such command:%s!\n", name);
        return;
    }

    option = (cli_opt_t*)malloc(sizeof(cli_opt_t));

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

static void cli_shell(cli_cmd_t *cmd)
{
#ifdef __linux__
    char     cmd_line[512];
    uint32_t offset     = 0;
    uint32_t argc       = cli_arg_cnt(cmd);
    uint32_t i          = 0;
    FILE     *pfile     = NULL;
    int32_t  read_cnt   = 0;
    uint32_t file_size  = 0;
    char     *strTemp   = NULL;
    int      ret_system = 0;
    const char *temp_file = ".temp_cmd_shell_output";

    if(argc <= 0){
        cli_output(cmd, "Please input shell cmd!\n");
        return ;
    }

    memset(cmd_line, 0, sizeof(cmd_line));

    for (i = 0; i < argc; i++) {
        snprintf(&(cmd_line[offset]), sizeof(cmd_line),
                 "%s ", cli_arg_get_str(cmd, i, ""));
        offset = strlen(cmd_line);
    }

    snprintf(&(cmd_line[offset]), sizeof(cmd_line), ">%s", temp_file);
    ret_system = system(cmd_line);
    if(ret_system < 0){
        cli_output(cmd, "System cmd run failed:%s\n", strerror(errno));
    }

    file_size = ex_file_size(temp_file) + 1;
    strTemp = (char*)malloc(file_size);

    /*  读取shell命令输出内容，并拷贝到缓存 */
    pfile = fopen(temp_file, "rb");

    read_cnt = ex_fread(pfile, strTemp, file_size - 1);
    strTemp[read_cnt] = '\0';
    cli_output(cmd, "%s", strTemp);

    free(strTemp);
    ex_fclose(pfile);

    /*  删除保存结果临时文件 */
    snprintf(cmd_line, sizeof(cmd_line), "rm -f %s", temp_file);
    ret_system = system(cmd_line);
    if(ret_system < 0){
        cli_output(cmd, "System cmd run failed:%s\n", strerror(errno));
    }
#else
    cli_output(cmd, "Only Support Linux Platform\n");
#endif
}

static void cli_quit(cli_cmd_t *cmd)
{
    int i = 0;
    g_is_quit = true;

    for(i = 0; i < g_quit_cb_idx; ++i) {
        if(g_quit_cbs[i]) {
            g_quit_cbs[i]();
        }
    }

    sleep_ms(TIME_100MS * 2);
    cli_output(cmd, "Bye!\n");
}

static void cli_ls(cli_cmd_t *cmd)
{
    chash_iter *itor = chash_iter_new(g_clis);
    cli_t *cli = NULL;
    cli_t *cli_alias = NULL;

    cli_output(cmd, "%-20s%s\n", "Name", "Brief");
    cli_output(cmd, "--------------------------------------------------\n");
    while(!chash_iter_is_end(itor)){
        cli = chash_iter_value(itor);

        if(cli->alias) {
            cli_alias = (cli_t*)chash_str_get(g_clis, cli->alias);
        } else {
            cli_alias = cli;
        }

        if(cli){
            cli_output(cmd, "%-20s%s\n", cli->name, cli_alias->desc);
        }

        chash_iter_next(itor);
    }

    chash_iter_free(itor);
}

void  cli_init(void)
{
    g_opts        = chash_new();
    g_clis        = chash_new();
    g_str_welcome = cstr_new();
    g_str_prompt  = cstr_new();

    cstr_append(g_str_prompt, "#");

    cli_regist("ls",    cli_ls);
    cli_regist("quit",  cli_quit);
    cli_regist("shell", cli_shell);
    cli_regist_alias("q", "quit");

    cli_set_brief("ls", "Show commands list.");
    cli_set_brief("quit", "Quit system.");
    cli_set_brief("shell", "Run linux shell command.");

    cli_enable_option("shell", false);
}

void  cli_release(void)
{
    free(g_str_prompt_extra);
    cstr_free(g_str_prompt);
    cstr_free(g_str_welcome);
    chash_free(g_clis);
    chash_free(g_opts);
}
