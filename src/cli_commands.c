/* {{{
 * =============================================================================
 *      Filename    :   cli_commands.c
 *      Description :
 *      Created     :   2015-09-02 08:09:00
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include "cli_commands.h"
#include "command.h"
#include "cobj_str.h"
#include "cobj_addr.h"

static dev_addr_t *cli_get_node_dev_addr(cli_cmd_t *cmd)
{
    const char *node = cli_opt_get_str(cmd, "node", "");
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

    dev_addr = cli_get_node_dev_addr(cmd);
    if(!dev_addr){
        return;
    }

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

static void cli_request(cli_cmd_t *cmd)
{

}

static void cli_dev_info(cli_cmd_t *cmd, const dev_addr_t *dev_addr)
{
    cli_output(cmd, "Name: %s Type: %d\n", dev_addr->name, dev_addr->type_dev);
    cli_output(cmd, "\tAddr NET: Addr Mac:\n");
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
    cli_regist("request", cli_request);
    cli_regist("sh",   cli_sh);
    cli_regist("exit", cli_exit);
    cli_regist("info", cli_info);

    cli_add_option("ping", "-n", "--node <node name>",
                   "remote node name you want to ping", NULL);
    cli_regist_alias("p", "ping");
}
