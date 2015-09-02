#ifndef COMMAND_TYPEDEF_H_201508281008
#define COMMAND_TYPEDEF_H_201508281008
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   command_typedef.h
 *      Description :
 *      Created     :   2015-08-28 10:08:18
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define DEV_NAME_MAX_LEN 16
#define ROUTER_NODE_MAX_CNT 64
#define ROUTER_LIST_MAX_CNT 4

typedef uint16_t cmd_code_t;
typedef uint16_t cmd_idx_t;
typedef uint32_t addr_mac_t;
typedef uint16_t addr_net_t;

typedef enum network_type_e
{
    NETWORK_TYPE_ENDPOINT = 0,
    NETWORK_TYPE_ROUTER,
    NETWORK_TYPE_CENTER
} network_type_t;

typedef struct network_node_s
{
    char dev_name[DEV_NAME_MAX_LEN];
    addr_mac_t addr_mac;
    // addr_net_t addr_net;
    uint8_t subnet_cnt;
    network_type_t network_type:8;
    uint16_t dev_type;
} network_node_t;

typedef struct router_path_s
{
    bool is_used;
    uint32_t nodes_cnt;
    time_t  time_add;
    network_node_t nodes[ROUTER_NODE_MAX_CNT];
} router_path_t;

#define SUBNET_MAX_CNT 4
typedef struct broadcast_msg_s
{
    uint8_t  version;
    uint8_t  router_list_cnt;
    uint16_t router_cnt;
    char dev_name[DEV_NAME_MAX_LEN];
    uint8_t router_list_lens[ROUTER_LIST_MAX_CNT];
    network_node_t network_nodes[ROUTER_NODE_MAX_CNT];
} broadcast_msg_t;

#ifdef __cplusplus
}
#endif
#endif  /* COMMAND_TYPEDEF_H_201508281008 */

