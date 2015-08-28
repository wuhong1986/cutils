#ifndef COBJ_CMD_RECV_H_201504230904
#define COBJ_CMD_RECV_H_201504230904
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   cobj_cmd_recv.h
 *      Description :
 *      Created     :   2015-04-23 09:04:08
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include <stdint.h>
#include "cobj.h"

/**
 * @Brief  接收命令节点、列表结构体
 */
typedef struct cmd_recv_s
{
    COBJ_HEAD_VARS;

    struct cmd_s   *cmd;
    uint32_t length;
    uint8_t *data;  /* 只是为了保存数据，
                       让命令处理程序可以访问，
                       当处理完命令后释放 */
}cmd_recv_t;

cmd_recv_t *cobj_cmd_recv_new(struct cmd_s *cmd, uint8_t *data, uint32_t data_len);

#ifdef __cplusplus
}
#endif
#endif  /* COBJ_CMD_RECV_H_201504230904 */

