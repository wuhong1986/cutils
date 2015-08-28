/* {{{
 * =============================================================================
 *      Filename    :   dev_addr.c
 *      Description :
 *      Created     :   2014-09-08 16:09:17
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "clist.h"
#include "csem.h"
#include "dev_addr.h"
#include "command.h"
#include "clog.h"
#include "cobj_str.h"

typedef struct dev_addr_mgr_s {
    uint16_t type_dev;
    uint8_t  sub_card;  /* 子卡号 */

    clist *list_devs_addr;  /* 仪器列表 */
#if 0
    chash *router_table;    /* 路由表 key: Mac Value: Router*/
#endif

    addr_type_t addr_types[ADDR_TYPE_MAX_CNT];
    bool  is_support[DEV_TYPE_MAX_CNT];    /* 支持的仪器类型 */
}dev_addr_mgr_t;

dev_addr_mgr_t dev_addr_mgr;
callback_process_dev_addr_offline cb_dev_addr_offline = NULL;

inline static void list_dev_addr_lock(void)
{
    clist_lock(dev_addr_mgr.list_devs_addr);
}

inline static void list_dev_addr_unlock(void)
{
    clist_unlock(dev_addr_mgr.list_devs_addr);
}

#if 0
inline static void router_table_lock(void)
{
    chash_lock(dev_addr_mgr.router_table);
}

inline static void router_table_unlock(void)
{
    chash_unlock(dev_addr_mgr.router_table);
}
#endif

void addr_type_set_name(uint8_t type, const char *name)
{
    if(NULL != dev_addr_mgr.addr_types[type].name) {
        free(dev_addr_mgr.addr_types[type].name);
    }
    dev_addr_mgr.addr_types[type].name = strdup(name);
}

void addr_type_set_prior(uint8_t type, uint8_t prior)
{
    dev_addr_mgr.addr_types[type].prior = prior;
}

void addr_type_set_ops(uint8_t type, const addr_ops_t *ops)
{
    dev_addr_mgr.addr_types[type].ops = *ops;
}

#if 0
void dev_addr_mgr_router_set(uint32_t mac, uint8_t cnt, uint32_t *path)
{
    addr_router_t *router = NULL;

    router_table_lock();

    router = (addr_router_t*)chash_int_get(dev_addr_mgr.router_table, mac);
    if(router) {
        dev_addr_router_init(router, cnt, path);
    } else {
        router = dev_addr_router_new(cnt, path);
        chash_int_set(dev_addr_mgr.router_table, mac, router);
    }

    router_table_unlock();
}

void dev_addr_mgr_router_del(uint32_t mac)
{
    router_table_lock();
    chash_int_del(dev_addr_mgr.router_table, mac);
    router_table_unlock();
}

bool dev_addr_mgr_router_exist(uint32_t mac)
{
    router_table_lock();
    return chash_int_haskey(dev_addr_mgr.router_table, mac);
}

const addr_router_t* dev_addr_mgr_router_get(uint32_t mac)
{
    return (addr_router_t*)chash_int_get(dev_addr_mgr.router_table, mac);
}
#endif

static dev_addr_t* dev_addr_mgr_find(const char *name)
{
    clist_node *node = NULL;
    dev_addr_t *dev_addr = NULL;

    clist_foreach_val(dev_addr_mgr.list_devs_addr, node, dev_addr){
        if(strcmp(dev_addr_get_name(dev_addr), name) == 0) {
            return dev_addr;
        }
    }

    return NULL;
}

dev_addr_t* dev_addr_mgr_get(const char *name)
{
    dev_addr_t *dev_addr = NULL;

    list_dev_addr_lock();

    dev_addr = dev_addr_mgr_find(name);

    list_dev_addr_unlock();

    return dev_addr;
}

dev_addr_t* dev_addr_mgr_add(const char *name, uint16_t type_dev)
{
    dev_addr_t *dev_addr = NULL;

    list_dev_addr_lock();

    dev_addr = dev_addr_mgr_find(name);
    if(NULL == dev_addr){
        dev_addr = dev_addr_new(name, type_dev);

        clist_append(dev_addr_mgr.list_devs_addr, dev_addr);
        // log_dbg("dev addr mgr add %s", name);
    } else {
        dev_addr_set_type_dev(dev_addr, type_dev);
    }

    list_dev_addr_unlock();

    return dev_addr;
}

void   dev_addr_mgr_set_offline_callback(callback_process_dev_addr_offline cb)
{
    cb_dev_addr_offline = cb;
}

void   dev_addr_mgr_proc_offline(addr_t *addr)
{
    dev_addr_t *dev_addr = NULL;

    if(NULL != cb_dev_addr_offline) {
        list_dev_addr_lock();

        dev_addr = dev_addr_mgr_find(addr_get_dev_name(addr));
        cb_dev_addr_offline(dev_addr);

        list_dev_addr_unlock();
    }
}

uint32_t dev_addr_mgr_get_cnt(void)
{
    return clist_size(dev_addr_mgr.list_devs_addr);
}

static clist *dev_addr_mgr_get_name_list_config(bool only_online)
{
    clist      *name_list    = NULL;
    clist_node *node         = NULL;
    dev_addr_t *dev_addr = NULL;
    bool       is_add        = true;
    cobj_str  *obj_name     = NULL;

    name_list = clist_new();

    list_dev_addr_lock();

    clist_foreach_val(dev_addr_mgr.list_devs_addr, node, dev_addr) {
        is_add = true;
        if(only_online && !dev_addr_is_online(dev_addr)) {
            is_add = false;
        }

        if(is_add) {
            obj_name = cobj_str_new(dev_addr_get_name(dev_addr));
            clist_append(name_list, obj_name);
        }
    }

    list_dev_addr_unlock();

    return name_list;
}

clist *dev_addr_mgr_get_name_list(void)
{
    return dev_addr_mgr_get_name_list_config(false);
}

clist *dev_addr_mgr_get_online_name_list(void)
{
    return dev_addr_mgr_get_name_list_config(true);
}

void   dev_addr_mgr_free_name_list(clist *name_list)
{
    clist_free(name_list);
}

addr_t* dev_addr_get_addr(dev_addr_t *dev_addr, uint8_t type)
{
    addr_t *addr = NULL;

    list_dev_addr_lock();

    addr = dev_addr_find_addr(dev_addr, type, NULL);

    list_dev_addr_unlock();

    return addr;
}

addr_t* dev_addr_add(dev_addr_t *dev_addr, uint8_t type, const void *obj_addr_info)
{
    addr_t *addr = NULL;
    addr_type_t *addr_type = NULL;

    list_dev_addr_lock();

    addr_type = &(dev_addr_mgr.addr_types[type]);

    addr = dev_addr_find_addr(dev_addr, type, obj_addr_info);
    if(NULL == addr){
        addr = addr_new(dev_addr, addr_type, cobj_dup(obj_addr_info));
        dev_addr_add_addr(dev_addr, addr);

        log_dbg("%s add address, type:%s",
                dev_addr_get_name(dev_addr), addr_type->name);
    } else {
        /* addr_type->ops.cb_free(info); */
    }

    /* 判断是否可用，不可用则进行连接 */
    if(  !addr_type->ops.cb_available(addr_get_addr_info(addr))
       && addr_type->ops.cb_connect) {
        /* printf("connect to remote!"); */
        addr_type->ops.cb_connect(addr, addr_get_addr_info(addr));
    }

    list_dev_addr_unlock();

    return addr;
}

void dev_addr_mgr_set_dev_type(uint16_t type)
{
    dev_addr_mgr.type_dev = type;
}

uint16_t dev_addr_mgr_get_dev_type(void)
{
    return dev_addr_mgr.type_dev;
}

void dev_addr_mgr_init(void)
{
    uint8_t i = 0;

    dev_addr_mgr.type_dev = DEV_TYPE_UNKNOWN;
    dev_addr_mgr.list_devs_addr = clist_new();

    for (i = 0; i < ADDR_TYPE_MAX_CNT; i++) {
        dev_addr_mgr.addr_types[i].type = i;
    }

    for (i = 0; i < DEV_TYPE_MAX_CNT; i++) {
        dev_addr_mgr.is_support[i] = false;
    }
}

void dev_addr_mgr_release(void)
{
    clist_free(dev_addr_mgr.list_devs_addr);
}

uint16_t dev_addr_mgr_type(void)
{
    return dev_addr_mgr.type_dev;
}

void dev_addr_mgr_add_support_dev_type(uint16_t type_dev)
{
    dev_addr_mgr.is_support[type_dev] = true;
}

bool dev_addr_mgr_is_support_dev_type(uint16_t type_dev)
{
    return dev_addr_mgr.is_support[type_dev];
}

const char *dev_type_name(uint8_t type_dev)
{
    switch(type_dev) {
    case DEV_TYPE_PIP:
        return "PIP";
    case DEV_TYPE_TEM_6CH:
        return "TEM6CH";
    case DEV_TYPE_TEM_1CH:
        return "TEM1CH";
    case DEV_TYPE_GSA:
        return "GSA";
    case DEV_TYPE_TRANS:
        return "TRANS";
    case DEV_TYPE_CENTER:
        return "Center";
    default:
        break;
    }

    return "Undefined";
}
