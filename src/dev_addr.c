/* {{{
 * =============================================================================
 *      Filename    :   dev_addr.c
 *      Description :
 *      Created     :   2015-04-23 16:04:52
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include <string.h>
#include "dev_addr.h"
#include "command.h"
#include "cobj_addr.h"
#include "network.h"
#include "clog.h"
#include "ex_memory.h"

static void dev_addr_free(void *obj)
{
    dev_addr_t *dev_addr = (dev_addr_t*)obj;

    cmutex_free(dev_addr->mutex_cmd_idx);
    clist_free(dev_addr->list_addr);
    free(dev_addr->name);
}

static cobj_ops_t cobj_ops_dev_addr = {
    .name = "Addr",
    .obj_size = sizeof(dev_addr_t),
    .cb_destructor = dev_addr_free,
};

dev_addr_t* dev_addr_new(const char *name, uint16_t type_dev, uint8_t subnet_cnt)
{
    uint8_t i = 0;
    dev_addr_t *dev_addr = ex_malloc_one(dev_addr_t);

    cobj_set_ops(dev_addr, &cobj_ops_dev_addr);

    dev_addr->name      = strdup(name);
    dev_addr->type_dev  = type_dev;
    dev_addr->subnet_cnt = subnet_cnt;
    dev_addr->list_addr = clist_new();
    dev_addr->cmd_idx   = 0;
    dev_addr->mutex_cmd_idx = cmutex_new();

    if(NETWORK_TYPE_CENTER == dev_addr_mgr_get_network_type()){
        for(i = 0; i < subnet_cnt; ++i) {
            dev_addr->subnet_addrs[i] = network_alloc_subnet();
            log_dbg("Alloc subnet address %04X:%02X for %s-%d",
                    network_get_subnet_net(dev_addr->subnet_addrs[i]),
                    network_get_subnet_idx(dev_addr->subnet_addrs[i]),
                    dev_addr->name, i);
        }
    }

    return dev_addr;
}

void dev_addr_add_addr(dev_addr_t *dev_addr, struct addr_s *addr)
{
    clist_prepend(dev_addr->list_addr, addr);
}

void dev_addr_set_type_dev(dev_addr_t *dev_addr, uint16_t type_dev)
{
    dev_addr->type_dev = type_dev;
}

void dev_addr_set_network_type(dev_addr_t *dev_addr, network_type_t type)
{
    dev_addr->network_type = type;
}

network_type_t dev_addr_get_network_type(dev_addr_t *dev_addr)
{
    return dev_addr->network_type;
}

void dev_addr_set_addr_mac(dev_addr_t *dev_addr, addr_mac_t addr_mac)
{
    dev_addr->addr_mac = addr_mac;
    /* printf("addr mac:%08X\n", addr_mac); */
}

addr_mac_t dev_addr_get_addr_mac(const dev_addr_t *dev_addr)
{
    return dev_addr->addr_mac;
}

const char *dev_addr_get_name(const dev_addr_t *dev_addr)
{
    return dev_addr->name;
}

bool dev_addr_is_online(const dev_addr_t *dev_addr)
{
    clist_node *node = NULL;
    addr_t     *addr = NULL;

    clist_foreach_val(dev_addr->list_addr, node, addr) {
        if(addr_is_available(addr)) {
            return true;
        }
    }

    return false;
}

bool dev_addr_is_offline(const dev_addr_t *dev_addr)
{
    return !dev_addr_is_online(dev_addr);
}

addr_t* dev_addr_get_addr_available(dev_addr_t *dev_addr, uint8_t type)
{
    clist_node *node     = NULL;
    addr_t     *addr     = NULL;

    clist_foreach_val(dev_addr->list_addr, node, addr){
        if(addr_is_same_type(addr, type)
        && addr_is_available(addr)) {
            break;
        }
    }

    return addr;
}

bool dev_addr_is_available(const dev_addr_t *dev_addr, uint8_t type)
{
    clist_node *node     = NULL;
    addr_t     *addr     = NULL;

    clist_foreach_val(dev_addr->list_addr, node, addr) {
        if(addr_is_same_type(addr, type)
        && addr_is_available(addr)) {
            return true;
        }
    }

    return false;
}

bool dev_addr_is_tx_abnormal(const dev_addr_t *dev_addr)
{
    clist_node *node          = NULL;
    addr_t     *addr          = NULL;
    bool       is_tx_abnormal = false;

    clist_foreach_val(dev_addr->list_addr, node, addr) {
        if(addr_is_available(addr)) {
            return false;
        } else if(addr_is_tx_abnormal(addr)) {
            is_tx_abnormal = true;
        }
    }

    return is_tx_abnormal;
}

struct addr_s* dev_addr_get_addr_send(dev_addr_t *dev_addr)
{
    uint8_t    prior_high = 0;
    uint8_t    prior      = 0;
    clist_node *node      = NULL;
    addr_t     *addr      = NULL;
    addr_t     *addr_tx   = NULL;

    /* 查找当前优先级最高并且可以使用的地址 */
    clist_foreach_val(dev_addr->list_addr, node, addr) {
        if(addr_is_available(addr)) {
            prior = addr_get_prior(addr);
            if(prior > prior_high) {
                prior_high = prior;
                addr_tx    = addr;
            }
        }
    }

    return addr_tx;
}

int  dev_addr_send(dev_addr_t *dev_addr, const void *data, uint32_t len)
{
    addr_t     *addr      = NULL;

    addr = dev_addr_get_addr_send(dev_addr);

    if(NULL == addr) { return -ERR_DEV_ADDR_NO_AVAIABLE; }

    return addr_send(addr, data, len);
}

int  dev_addr_get_cnt(const dev_addr_t *dev_addr)
{
    return clist_count(dev_addr->list_addr);
}

uint32_t dev_addr_get_next_cmd_idx(dev_addr_t *dev_addr)
{
    uint32_t idx = 0;

    cmutex_lock(dev_addr->mutex_cmd_idx);

    ++(dev_addr->cmd_idx);
    if(dev_addr->cmd_idx == 0) {
        ++(dev_addr->cmd_idx);
    }

    idx = dev_addr->cmd_idx;

    cmutex_unlock(dev_addr->mutex_cmd_idx);

    return idx;
}

uint16_t dev_addr_get_type(const dev_addr_t *dev_addr)
{
    return dev_addr->type_dev;
}

addr_t* dev_addr_find_addr(dev_addr_t *dev_addr, uint8_t type,
                              const void *obj_addr_info)
{
    clist_node  *node      = NULL;
    addr_t      *addr      = NULL;
    addr_type_t *addr_type = NULL;

    clist_foreach_val(dev_addr->list_addr, node, addr){
        addr_type = addr_get_addr_type(addr);
        if(addr_type->type == type) {
            if(NULL == obj_addr_info) return addr;
            if(cobj_equal(obj_addr_info, addr_get_addr_info(addr))) {
                return addr;
            }
        }
    }

    return NULL;
}
