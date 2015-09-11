/* {{{
 * =============================================================================
 *      Filename    :   cli_commands.c
 *      Description :
 *      Created     :   2015-09-02 08:09:00
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include <string.h>
#include "cli_commands.h"
#include "command.h"
#include "cobj_str.h"
#include "cobj_addr.h"
#include "clog.h"
#include "ex_time.h"

static dev_addr_t *cli_get_node_dev_addr(cli_cmd_t *cmd)
{
    const char *node = cli_opt_get_str(cmd, "node", "1004");
    dev_addr_t *dev_addr = NULL;

    if(strcmp(node, "") == 0) {
        cli_output(cmd, "Please input option --node \n");
        return NULL;
    }

    if(!(dev_addr = dev_addr_mgr_get(node))) {
        cli_output(cmd, "No such device: %s\n", node);
        return NULL;
    } else {
        return dev_addr;
    }
}

static void cli_ping(cli_cmd_t *cmd)
{
    dev_addr_t *dev_addr = NULL;
    cmd_req_t *req = NULL;
    cmd_t     *response = NULL;

    dev_addr = dev_addr_mgr_get("1001");
    if(!dev_addr){
        cli_output(cmd, "No such device: %s\n", "1001");
        return;
    }

    req = cmd_new_req(dev_addr, CMD_CODE_PING);
    cmd_req_set_sync(req);

    uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    cmd_req_set_data(req, data, sizeof(data));

    if(S_OK == cmd_send_request(req)){
        if(cmd_recv_sync_response(req)) {
            cli_output(cmd, "Pong!\n");
        } else {
            cli_output(cmd, "Ping timeout!\n");
        }
    } else {
        cli_output(cmd, "Command send failed!\n");
    }

    cmd_req_free(req);
}

static void cli_echo(cli_cmd_t *cmd)
{
    dev_addr_t *dev_addr = NULL;
    cmd_req_t *req = NULL;
    cmd_t     *response = NULL;
    uint8_t   *data = NULL;
    uint8_t   *data_resp = NULL;
    uint32_t   argc = 0;
    uint32_t   i = 0;
    bool is_sync = true;
    bool is_test = false;
    uint32_t loop_cnt = 1;

    dev_addr = cli_get_node_dev_addr(cmd);
    if(!dev_addr){
        return;
    }

    is_sync = (strcmp(cli_opt_get_str(cmd, "mode", "sync"), "async") != 0);
    is_test = cli_opt_exist(cmd, "test");
    loop_cnt = cli_opt_get_int(cmd, "loop", 1);

    argc = cli_arg_cnt(cmd);
    if(argc <= 0) {
        cli_output(cmd, "Please input echo content.\n");
        return;
    }

    data = (uint8_t*)malloc(argc);

    for(i = 0; i < argc; ++i) {
        data[i] = cli_arg_get_hex(cmd, i, 0);
    }

    req = cmd_new_req(dev_addr, CMD_CODE_ECHO);
    cmd_req_set_sync(req);

    cmd_req_set_data(req, data, argc);

    cmd_send_request(req);

    response = cmd_recv_sync_response(req);
    if(!response) {
        cli_output(cmd, "Echo timeout!\n");
    } else if(cmd_get_error(response) != S_OK) {
        cli_output(cmd, "Echo error, return code %d\n", cmd_get_error(response));
    } else {
        data_resp = cmd_get_data(response);
        cli_output(cmd, "Echo OK, response is:\n");
        for(i = 0; i < cmd_get_data_len(response); ++i) {
            cli_output(cmd, " %02X", data_resp[i]);
        }
        cli_output(cmd, "\n");
    }

    cmd_req_free(req);
    free(data);
}

static Status cmd_proc_resp_txtest(const cmd_req_t *cmd_req, const cmd_t *cmd_resp)
{
    uint32_t *recv_cnt = (uint32_t*)cmd_req->private_data;

    ++(*recv_cnt);

    return S_OK;
}

static void cli_txtest(cli_cmd_t *cmd)
{
    dev_addr_t *dev_addr = NULL;
    cmd_req_t *req = NULL;
    cmd_t     *response = NULL;
    uint32_t   *data = NULL;
    uint8_t   *data_resp = NULL;
    uint32_t   argc = 0;
    uint32_t   i = 0;
    uint32_t pack_size = 128;
    bool is_sync = true;
    bool is_test = false;
    uint32_t loop_cnt = 1;
    uint32_t recv_cnt = 0;
    struct timeval time_start;
    struct timeval time_end;
    struct timeval time_cost;

    dev_addr = cli_get_node_dev_addr(cmd);
    if(!dev_addr){
        return;
    }

    /* is_sync = (strcmp(cli_opt_get_str(cmd, "mode", "sync"), "async") != 0); */
    /* is_sync = false; */
    is_test = cli_opt_exist(cmd, "test");
    loop_cnt = cli_opt_get_int(cmd, "loop", 1);

    data = (uint32_t*)malloc(pack_size * sizeof(uint32_t));

    for(i = 0; i < pack_size; ++i) {
        data[i] = i;
    }

    gettimeofday(&time_start, NULL);

    loop_cnt = 5000;
    for(i = 0; i < loop_cnt; ++i) {
        /* printf("test %d\n", i); */
        req = cmd_new_req(dev_addr, CMD_CODE_TX_TEST);
        if(is_sync){
            cmd_req_set_sync(req);
        }

        req->private_data = &recv_cnt;
        cmd_req_set_data(req, data, pack_size * sizeof(uint32_t));

        cmd_send_request(req);

        if(is_sync){
            response = cmd_recv_sync_response(req);
            if(!response) {
                cli_output(cmd, "Echo timeout!\n");
            } else if(cmd_get_error(response) != S_OK) {
                cli_output(cmd, "Echo error, return code %d\n", cmd_get_error(response));
            } else {
#if 0
                data_resp = cmd_get_data(response);
                printf("Echo OK, response is:\n");
                cli_output(cmd, "Echo OK, response is:\n");
                for(i = 0; i < cmd_get_data_len(response); ++i) {
                    cli_output(cmd, " %02X", data_resp[i]);
                }
                cli_output(cmd, "\n");
#endif
            }

            cmd_req_free(req);
        }
    }

    int sleep_total = 0;
    while(recv_cnt != loop_cnt && sleep_total < TIME_1S && !is_sync){
        sleep_ms(1);
        sleep_total += 1;
    }
    gettimeofday(&time_end, NULL);

    timersub(&time_end, &time_start, &time_cost);

    cli_output_kv_start(cmd);
    cli_output_key_ivalue(cmd, "loop_cnt", loop_cnt);
    cli_output_kv_sep(cmd);
    cli_output_key_ivalue(cmd, "recv_cnt", recv_cnt);
    cli_output_kv_sep(cmd);
    cli_output_key_ivalue(cmd, "sleep", sleep_total);
    cli_output_kv_sep(cmd);
    cli_output_key_ivalue(cmd, "size", pack_size * sizeof(uint32_t));
    cli_output_kv_sep(cmd);
    cli_output_key_ivalue(cmd, "cost_sec", time_cost.tv_sec);
    cli_output_kv_sep(cmd);
    cli_output_key_ivalue(cmd, "cost_usec", time_cost.tv_usec);
    cli_output_kv_end(cmd);

    free(data);
}

static void cli_request(cli_cmd_t *cmd)
{

}

static void cli_dev_info(cli_cmd_t *cmd, const dev_addr_t *dev_addr)
{
    cli_output(cmd, "Name: %s Type: %d\n", dev_addr->name, dev_addr->type_dev);
    cli_output(cmd, "\tAddr NET: Addr Mac:0x%08X\n", dev_addr->addr_mac);
    cli_output(cmd, "\tAddress:\n");
    if(dev_addr->list_addr) {
        clist_iter iter = clist_begin(dev_addr->list_addr);
        addr_t *addr = NULL;
        clist_iter_foreach_obj(&iter, addr) {
            ;
        }
    }
    cli_output(cmd, "\n");
}

static void cli_info(cli_cmd_t *cmd)
{
    clist *namelist = dev_addr_mgr_get_name_list();
    clist_iter iter = clist_begin(namelist);
    cobj_str *obj_str = NULL;

    clist_iter_foreach_obj(&iter, obj_str) {
        const dev_addr_t *dev_addr = dev_addr_mgr_get(cobj_str_val(obj_str));
        cli_dev_info(cmd, dev_addr);
    }

    clist_free(namelist);
}

static void cli_sh(cli_cmd_t *cmd)
{
    const char *node = cli_arg_get_str(cmd, 0, "");

    cli_set_default_opt("node", node);
    if(node && strcmp(node, "") != 0){
        cli_set_extra_prompt(node);
    } else {
        cli_output(cmd, "Please input node name\n");
    }
}

static void cli_exit(cli_cmd_t *cmd)
{
    cli_set_default_opt("node", "");
    cli_set_extra_prompt("");
}

void cli_commands_init(void)
{
    cli_regist("ping", cli_ping);
    cli_regist("echo", cli_echo);
    cli_regist("txtest", cli_txtest);
    cli_regist_alias("t", "txtest");
    cli_regist("request", cli_request);
    cli_regist("sh",   cli_sh);
    cli_regist("exit", cli_exit);
    cli_regist("info", cli_info);

    cli_add_option("txtest", "-m", "--mode", "sync(default) or async.", NULL);
    cli_add_option("txtest", "-t", "--test <max number value>",
                   "enable test mode", NULL);
    cli_add_option("txtest", "-l", "--loop <loop count>", "loop cnt", NULL);

    cli_add_option("ping", "-n", "--node <node name>",
                   "remote node name you want to ping", NULL);
    cli_regist_alias("p", "ping");

    cmd_routine_regist_resp(CMD_CODE_TX_TEST, cmd_proc_resp_txtest, NULL);
}
