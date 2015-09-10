/* {{{
 * =============================================================================
 *      Filename    :   test_main.c
 *      Description :
 *      Created     :   2015-08-26 09:08:15
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include <stdlib.h>
#include <stdio.h>
#include "ccli.h"
#include "cli_commands.h"
#include "command.h"
#include "cthread.h"
#include "clog.h"
#include "socket_broadcast.h"
#include "ex_memory.h"
#include "dev_addr_mgr.h"
#include "dev_router.h"

int main(int argc, char *argv[])
{
    cli_init();
    log_init();
    thread_init();
    cmd_init();
    dev_addr_mgr_init();
    addr_sock_init();
    socket_init();
    dev_router_init();
    cli_commands_init();

    broadcast_msg_t msg;
    dev_addr_mgr_add_support_dev_type(1);
    dev_addr_mgr_set_network_type(NETWORK_TYPE_CENTER);
    dev_addr_mgr_set_addr_mac(0x89ABCDE0);

    dev_router_set_mac_local(0x89ABCDE0);

    ex_memzero_one(&msg);
    strcpy(msg.dev_name, "1004");
    msg.router_list_lens[0] = 3;
    msg.router_list_lens[1] = 3;
    msg.router_list_lens[2] = 0;
    msg.router_list_lens[3] = 0;
    msg.router_cnt = 7;
    msg.router_list_cnt = 2;
    msg.network_nodes[0].dev_type = 1;
    msg.network_nodes[0].addr_mac = 0x89ABCDE0;
    msg.network_nodes[0].subnet_cnt = 4;
    msg.network_nodes[0].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[0].dev_name, "1004");

    msg.network_nodes[1].dev_type = 1;
    msg.network_nodes[1].addr_mac = 0x89ABCDE1;
    msg.network_nodes[1].subnet_cnt = 4;
    msg.network_nodes[1].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[1].dev_name, "1003");

    msg.network_nodes[2].dev_type = 1;
    msg.network_nodes[2].addr_mac = 0x89ABCDE2;
    msg.network_nodes[2].subnet_cnt = 4;
    msg.network_nodes[2].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[2].dev_name, "1002");

    msg.network_nodes[3].dev_type = 1;
    msg.network_nodes[3].addr_mac = 0x89ABCDE3;
    msg.network_nodes[3].subnet_cnt = 4;
    msg.network_nodes[3].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[3].dev_name, "1001");

    msg.network_nodes[4].dev_type = 1;
    msg.network_nodes[4].addr_mac = 0x89ABCDE4;
    msg.network_nodes[4].subnet_cnt = 4;
    msg.network_nodes[4].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[4].dev_name, "1005");

    msg.network_nodes[5].dev_type = 1;
    msg.network_nodes[5].addr_mac = 0x89ABCDE5;
    msg.network_nodes[5].subnet_cnt = 4;
    msg.network_nodes[5].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[5].dev_name, "1006");

    msg.network_nodes[6].dev_type = 1;
    msg.network_nodes[6].addr_mac = 0x89ABCDE6;
    msg.network_nodes[6].subnet_cnt = 4;
    msg.network_nodes[6].network_type = NETWORK_TYPE_ROUTER;
    strcpy(msg.network_nodes[6].dev_name, "1007");

    socket_listen_async(50002);
    socket_listen_cli(49999);
    socket_recv_start();
    socket_bc_tx_start("test", 50000, 50001, 50002);
    socket_bc_rx_start("test", 50000, 50001, &msg);

#if 0
    cstr *json = cstr_new();
    int fd = 0;
    fd = socket_cli_send_request("127.0.0.1", 49999, "test");
    socket_cli_recv_response(fd, json);
    cstr_clear(json);
    fd = socket_cli_send_request("127.0.0.1", 49999, "test fuck");
    socket_cli_recv_response(fd, json);
    cstr_clear(json);
    fd = socket_cli_send_request("127.0.0.1", 49999, "fuck fuck fuck you test fuck");
    socket_cli_recv_response(fd, json);
    cstr_free(json);
#endif

    cli_loop();

    dev_addr_mgr_release();
    socket_release();
    cmd_release();
    thread_release();
    log_release();
    cli_release();
#if 0
  cli_cmd_t cmd;
  int i = 0;

  command_init(&cmd, argv[0], "0.0.1");
  command_option(&cmd, "-v", "--verbose", "enable verbose stuff", verbose);
  command_option(&cmd, "-r", "--required <arg>", "required arg", required);
  command_option(&cmd, "-o", "--optional [arg]", "optional arg", optional);
  command_parse(&cmd, argc, argv);
  printf("additional args:\n");
  for (i = 0; i < cmd.argc; ++i) {
    printf("  - '%s'\n", cmd.argv[i]);
  }
  command_free(&cmd);
#endif
    return 0;
}

