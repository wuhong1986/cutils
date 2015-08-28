#ifndef QJ_COMM_SOCK_H_201310100910
#define QJ_COMM_SOCK_H_201310100910
#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 *      Filename    :   qj_comm_sock.h
 *      Description :   主要负责Socket 通信
 *      Created     :   2013-10-10 09:10:23
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <stdint.h>
#include "ex_errno.h"

Status  socket_server_start(uint16_t port);
Status  socket_recv_start(void);
void socket_init(void);
void socket_release(void);

#ifdef __cplusplus
}
#endif
#endif  /* QJ_COMM_SOCK_H_201310100910 */
