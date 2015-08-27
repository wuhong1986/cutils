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

void cli_hello(cli_cmd_t *cmd)
{
    printf("Hello\n");
}

int main(int argc, char *argv[])
{
    cli_init();

    cli_t *cli = cli_regist("h", cli_hello);
    cli_add_option(cli, "-v", "--verbose <arg>", "fuck haha", NULL);
    cli_loop();

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

