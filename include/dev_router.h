#ifndef DEV_ROUTER_H_201509010909
#define DEV_ROUTER_H_201509010909
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   dev_router.h
 *      Description :
 *      Created     :   2015-09-01 09:09:20
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include "command_typedef.h"

void dev_router_add_path(const network_node_t *node_head,
                         const network_node_t *nodes, uint32_t nodes_cnt);

void dev_router_set_mac_local(addr_mac_t mac_local);
void dev_router_init(void);
#ifdef __cplusplus
}
#endif
#endif  /* DEV_ROUTER_H_201509010909 */

