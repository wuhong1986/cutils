/* {{{
 * =============================================================================
 *      Filename    :   network.c
 *      Description :
 *      Created     :   2015-08-31 15:08:33
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include "network.h"

static addr_net_t g_addr_net_last = 0;

uint32_t network_get_subnet_net(addr_net_t addr_net)
{
    return addr_net >> 7;
}

uint32_t network_get_subnet_idx(addr_net_t addr_net)
{
    return addr_net & 0x7F;
}

addr_net_t network_alloc_subnet(void)
{
    return (++g_addr_net_last) << 7;
}


