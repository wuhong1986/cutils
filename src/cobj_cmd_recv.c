/* {{{
 * =============================================================================
 *      Filename    :   cobj_cmd_recv.c
 *      Description :
 *      Created     :   2015-04-23 09:04:04
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include "cobj_cmd_recv.h"
#include "command.h"

static void cobj_cmd_recv_free(void *val)
{
    cmd_recv_t *cmd_recv = (cmd_recv_t*)val;
    cmd_t      *cmd      = NULL;

    if(NULL == cmd_recv) return ;

    cmd = cmd_recv->cmd;
    if(CMD_BODY_TYPE_DATA == cmd_get_body_type(cmd)) {
        cmd->body.data = NULL;
    }

    cmd_free(cmd);
    free(cmd_recv->data);
}

static cobj_ops_t cobj_ops_cmd_recv = {
    .name = "CmdRecv",
    .obj_size = sizeof(cmd_recv_t),
    .cb_destructor = cobj_cmd_recv_free,
};

cmd_recv_t *cobj_cmd_recv_new(struct cmd_s *cmd, uint8_t *data, uint32_t data_len)
{
    cmd_recv_t *cmd_recv = (cmd_recv_t*)malloc(sizeof(cmd_recv_t));

    cobj_set_ops(cmd_recv, &cobj_ops_cmd_recv);

    cmd_recv->cmd    = cmd;
    cmd_recv->length = data_len;
    cmd_recv->data   = data;

    return cmd_recv;
}

