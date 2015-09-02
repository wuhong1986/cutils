#ifndef SOCKET_BROADCAST_H_201410061510
#define SOCKET_BROADCAST_H_201410061510
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   socket_broadcast.h
 *      Description :
 *      Created     :   2014-10-06 15:10:32
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

#include <stdint.h>
#include "errno.h"
#include "command_typedef.h"

Status socket_bc_tx_start(const char *name, uint16_t port_bc,
                          uint16_t port_resp, uint16_t port_connect);
Status socket_bc_rx_start(const char *name,
                          uint16_t port_bc, uint16_t port_resp,
                          const broadcast_msg_t *msg);

#ifdef __cplusplus
}
#endif
#endif  /* SOCKET_BROADCAST_H_201410061510 */
