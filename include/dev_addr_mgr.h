#ifndef DEV_ADDR_H_201409081609
#define DEV_ADDR_H_201409081609
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   dev_addr.h
 *      Description :
 *      Created     :   2014-09-08 16:09:11
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

#include <stdbool.h>
#include <stdint.h>
#include "clist.h"
#include "cobj_addr.h"
#include "dev_addr.h"
// #include "cobj_dev_addr_router.h"

#define ADDR_TYPE_ETHERNET      1   /* 以太网 */
#define ADDR_TYPE_ZIGBEE        2   /* Zigbee */
#define ADDR_TYPE_BLUETOOTH     3   /* 蓝牙 */
#define ADDR_TYPE_UART          4   /* 串口 */
#define ADDR_TYPE_MAX_CNT       16

#define ADDR_PRIOR_ETHERNET     250  /*  */
#define ADDR_PRIOR_ZIGBEE       200
#define ADDR_PRIOR_BLUETOOTH    190

#define DEV_TYPE_UNKNOWN    (0)
#define DEV_TYPE_TRANS      (1)     /* 非具体的仪器，用于文件传输和自动升级 */
#define DEV_TYPE_PIP        (2)
#define DEV_TYPE_TEM_6CH    (3)
#define DEV_TYPE_GSA        (4)
#define DEV_TYPE_CENTER     (5)
#define DEV_TYPE_PRINTER    (6)
#define DEV_TYPE_TEM_TRANS  (7)
#define DEV_TYPE_TEM_1CH    (8)
#define DEV_TYPE_TEM        (9)
#define DEV_TYPE_CSAMT      (10)
#define DEV_TYPE_AMT        (11)
#define DEV_TYPE_MT         (12)
#define DEV_TYPE_MAX_CNT    (32)

#define ERR_DEV_ADDR_START  900
#define ERR_DEV_ADDR_NO_AVAIABLE    (ERR_DEV_ADDR_START + 0)

typedef struct addr_type_s addr_type_t;

typedef void (*callback_process_dev_addr_offline)(dev_addr_t *dev_addr);

typedef int   (*callback_addr_send)(void *obj_addr_info,
                                    const void *data, uint32_t len);
typedef bool  (*callback_addr_available)(void *obj_addr_info);
typedef bool  (*callback_addr_tx_abnormal)(void *obj_addr_info);
typedef void  (*callback_addr)(void *obj_addr_info);
typedef void  (*callback_addr_connect)(addr_t *addr, void *obj_addr_info);

typedef struct addr_ops_s {
    callback_addr_connect   cb_connect; /* 连接 */
    callback_addr           cb_disconnect;  /* 断开连接 */
    callback_addr_send      cb_send;        /* 发送数据 */
    callback_addr_available cb_available;   /* 是否可用 */
    callback_addr_tx_abnormal cb_abnormal;
    callback_addr   cb_after_offline;
}addr_ops_t;

struct addr_type_s {
    const char *name;
    uint8_t type;   /* 地址类型，相同类型地址ops 必须一样 */
    uint8_t prior;  /* 优先级 */
    addr_ops_t ops; /* 该地址的一些回调函数 */
};

typedef struct dev_addr_mgr_s {
    uint16_t type_dev;
    network_type_t network_type;
    uint8_t  sub_card;  /* 子卡号 */

    addr_mac_t addr_mac;

    clist *list_devs_addr;  /* 仪器列表 */
#if 0
    chash *router_table;    /* 路由表 key: Mac Value: Router*/
#endif

    addr_type_t addr_types[ADDR_TYPE_MAX_CNT];
    bool  is_support[DEV_TYPE_MAX_CNT];    /* 支持的仪器类型 */
}dev_addr_mgr_t;

struct addr_router_s;
#ifdef CMD_ENABLE_ROUTER

#endif

void addr_type_set_name(uint8_t type, const char *name);
void addr_type_set_prior(uint8_t type, uint8_t prior);
void addr_type_set_ops(uint8_t type, const addr_ops_t *ops);

const char *dev_type_name(uint8_t type_dev);

void dev_addr_mgr_init(void);
void dev_addr_mgr_release(void);
uint16_t dev_addr_mgr_type(void);
void dev_addr_mgr_set_dev_type(uint16_t type);
uint16_t dev_addr_mgr_get_dev_type(void);
void dev_addr_mgr_add_support_dev_type(uint16_t type_dev);
bool dev_addr_mgr_is_support_dev_type(uint16_t type_dev);
void dev_addr_mgr_set_network_type(network_type_t type);
network_type_t dev_addr_mgr_get_network_type(void);
dev_addr_t* dev_addr_mgr_add(const char *name, uint16_t type_dev, uint8_t subnet_cnt);
dev_addr_t* dev_addr_mgr_get(const char *name);
void dev_addr_mgr_set_addr_mac(addr_mac_t addr_mac);
addr_mac_t dev_addr_mgr_get_addr_mac(void);
uint32_t dev_addr_mgr_get_cnt(void);
clist *dev_addr_mgr_get_name_list(void);
clist *dev_addr_mgr_get_online_name_list(void);
void   dev_addr_mgr_free_name_list(clist *name_list);
void   dev_addr_mgr_proc_offline(addr_t *addr);
void   dev_addr_mgr_set_offline_callback(callback_process_dev_addr_offline cb);

void dev_addr_mgr_router_set(uint32_t mac, uint8_t cnt, uint32_t *path);
void dev_addr_mgr_router_del(uint32_t mac);
bool dev_addr_mgr_router_exist(uint32_t mac);
const struct addr_router_s* dev_addr_mgr_router_get(uint32_t mac);

#ifdef __cplusplus
}
#endif
#endif  /* DEV_ADDR_H_201409081609 */
