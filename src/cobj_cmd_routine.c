/* {{{
 * =============================================================================
 *      Filename    :   cobj_cmd_routine.c
 *      Description :
 *      Created     :   2015-04-23 11:04:18
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include "cobj_cmd_routine.h"

static cobj_ops_t cobj_ops_cmd_routine = {
    .name = "CmdRoutine",
    .obj_size = sizeof(cmd_routine_t),
};

cmd_routine_t* cobj_cmd_routine_new(cmd_code_t      cmd_code, cmd_callback_req  cb_req,
                                    cmd_callback_ot cb_ot,    cmd_callback_resp cb_resp)
{
    cmd_routine_t *routine = (cmd_routine_t*)malloc(sizeof(cmd_routine_t));

    cobj_set_ops(routine, &cobj_ops_cmd_routine);

    routine->code          = cmd_code;
    routine->callback_req  = cb_req;
    routine->callback_ot   = cb_ot;
    routine->callback_resp = cb_resp;

    return routine;
}

