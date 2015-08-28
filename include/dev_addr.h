#ifndef dev_addr_H_201504231604
#define dev_addr_H_201504231604
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   dev_addr.h
 *      Description :
 *      Created     :   2015-04-23 16:04:50
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include "cobj.h"
#include "csem.h"
#include "clist.h"
#include <stdint.h>

struct addr_s;
typedef struct dev_addr_s dev_addr_t;

struct dev_addr_s {
    COBJ_HEAD_VARS;

    char    *name;      /* 仪器名称 */
    uint16_t type_dev;  /* 仪器类型 */

    uint8_t level;  /* 第几级网络 */
    uint32_t mac;  /* 0.0.0.0 未设置 FF.FF.FF.FF 表示无效地址 */
    uint32_t mac_parent;

    cmutex  *mutex_cmd_idx;
    uint8_t  cmd_idx;
    clist   *list_addr; /* 该仪器的地址信息列表，
                           一种仪器可包含多个地址信息比如Zigbee、WIFI等 */
};

dev_addr_t* dev_addr_new(const char *name, uint16_t type_dev);
void dev_addr_add_addr(dev_addr_t *dev_addr, struct addr_s *addr);

void dev_addr_set_type_dev(dev_addr_t *dev_addr, uint16_t type_dev);
const char *dev_addr_get_name(const dev_addr_t *dev_addr);
bool dev_addr_is_online(const dev_addr_t *dev_addr);
bool dev_addr_is_offline(const dev_addr_t *dev_addr);

struct addr_s* dev_addr_add(dev_addr_t *dev_addr, uint8_t type, const void *info);
struct addr_s* dev_addr_get_addr(dev_addr_t *dev_addr, uint8_t type);
struct addr_s* dev_addr_get_addr_available(dev_addr_t *dev_addr, uint8_t type);
struct addr_s* dev_addr_get_addr_send(dev_addr_t *dev_addr);

bool dev_addr_is_available(const dev_addr_t *dev_addr, uint8_t type);
bool dev_addr_is_tx_abnormal(const dev_addr_t *dev_addr);
int  dev_addr_send(dev_addr_t *dev_addr, const void *data, uint32_t len);
int  dev_addr_get_cnt(const dev_addr_t *dev_addr);
uint32_t dev_addr_get_next_cmd_idx(dev_addr_t *dev_addr);
uint16_t dev_addr_get_type(const dev_addr_t *dev_addr);

struct addr_s* dev_addr_find_addr(dev_addr_t *dev_addr, uint8_t type,
                         const void *obj_addr_info);

#ifdef __cplusplus
}
#endif
#endif  /* dev_addr_H_201504231604 */

