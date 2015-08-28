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
#include "command.h"
#include "cthread.h"
#include "clog.h"
#include "socket_broadcast.h"
#include "ex_memory.h"
#include "dev_addr_mgr.h"

void cli_hello(cli_cmd_t *cmd)
{
    printf("Hello\n");
}

int main(int argc, char *argv[])
{
    cli_init();
    log_init();
    thread_init();
    cmd_init();
    dev_addr_mgr_init();
    addr_sock_init();
    socket_init();

    socket_resp_msg_t msg;
    dev_addr_mgr_add_support_dev_type(1);

    ex_memzero_one(&msg);
    msg.dev_type = 1;
    strcpy(msg.dev_name, "1001");

    socket_server_start(50002);
    socket_recv_start();
    socket_bc_tx_start("test", 50000, 50001, 50002);
    socket_bc_rx_start("test", 50000, 50001, &msg);

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

