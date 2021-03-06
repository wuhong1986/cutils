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
#include "cstring.h"
#include "ex_errno.h"

void socket_listen_cli(uint16_t listen_port);
int socket_connect_cli(const char *ip, uint16_t port);
void socket_close_cli();
int socket_cli_send_request(const char *ip, uint16_t port, const char *request_str);
int socket_cli_recv_response(int fd, int msec, cstr *response);

void socket_listen_async(uint16_t listen_port);
Status  socket_recv_start(void);
void socket_init(void);
void socket_release(void);

int  bufferevent_send(struct bufferevent *bev, const void *data, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif  /* QJ_COMM_SOCK_H_201310100910 */
