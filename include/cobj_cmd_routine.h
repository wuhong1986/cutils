#ifndef COBJ_CMD_ROUTINE_H_201504231104
#define COBJ_CMD_ROUTINE_H_201504231104
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   cobj_cmd_routine.h
 *      Description :
 *      Created     :   2015-04-23 11:04:07
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include "cobj.h"
#include "ex_errno.h"
#include "command_typedef.h"

struct cmd_s;
struct cmd_req_s;
typedef Status (*cmd_callback_resp)(const struct cmd_req_s *cmd_req,
                                    const struct cmd_s *cmd_resp);
typedef Status (*cmd_callback_ot)(struct cmd_s *cmd);
typedef void   (*cmd_callback_req)(const struct cmd_s *cmd_req, struct cmd_s *cmd_resp);

/**
 * @Brief  命令以及对应回调函数结构体
 */
typedef struct cmd_routine_s{
    COBJ_HEAD_VARS;

	int               code;
	cmd_callback_req  callback_req;           /*  命令响应处理函数 */
    cmd_callback_ot   callback_ot;
    cmd_callback_resp callback_resp;
}cmd_routine_t;

cmd_routine_t* cobj_cmd_routine_new(cmd_code_t      cmd_code, cmd_callback_req  cb_req,
                                    cmd_callback_ot cb_ot,    cmd_callback_resp cb_resp);

#ifdef __cplusplus
}
#endif
#endif  /* COBJ_CMD_ROUTINE_H_201504231104 */

