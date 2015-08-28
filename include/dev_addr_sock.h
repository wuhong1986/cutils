#ifndef DEV_ADDR_SOCK_H_201409131409
#define DEV_ADDR_SOCK_H_201409131409
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   dev_addr_sock.h
 *      Description :
 *      Created     :   2014-09-13 14:09:53
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

#include <stdbool.h>
#include <stdint.h>
#include "event2/bufferevent.h"
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "cobj.h"
#include "dev_addr.h"

typedef struct addr_sock_s
{
    COBJ_HEAD_VARS;

    evutil_socket_t  fd;
    bool    is_fix_port;
    bool    is_tx_abnormal;
    uint32_t sock_addr_src;
    struct sockaddr_in sockaddr;
    struct bufferevent *bev;
}addr_sock_t;

void addr_sock_init(void);
void addr_sock_set_event_base(struct event_base *event_base);

addr_sock_t* cobj_addr_sock_new(void);

#ifdef __cplusplus
}
#endif
#endif  /* DEV_ADDR_SOCK_H_201409131409 */
