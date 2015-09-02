#ifndef NETWORK_H_201508311508
#define NETWORK_H_201508311508
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   network.h
 *      Description :
 *      Created     :   2015-08-31 15:08:33
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include "command_typedef.h"

uint32_t network_get_subnet_net(addr_net_t addr_net);
uint32_t network_get_subnet_idx(addr_net_t addr_net);

addr_net_t network_alloc_subnet(void);

#ifdef __cplusplus
}
#endif
#endif  /* NETWORK_H_201508311508 */

