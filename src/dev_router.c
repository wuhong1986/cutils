/* {{{
 * =============================================================================
 *      Filename    :   dev_router.c
 *      Description :
 *      Created     :   2015-09-01 09:09:14
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */
#include "dev_router.h"
#include "clog.h"
#include "ccli.h"

#define ROUTER_PATH_MAX_CNT 32
static router_path_t g_paths[ROUTER_PATH_MAX_CNT];
static addr_mac_t g_mac_local;

static void set_path(router_path_t *path, const network_node_t *node_head,
                     const network_node_t *nodes, uint32_t nodes_cnt)
{
    uint8_t i = 0;
    path->nodes[0] = *node_head;
    for(i = 0; i < nodes_cnt; ++i) {
        path->nodes[i + 1] = nodes[i];
    }

    path->nodes_cnt = nodes_cnt + 1;
    path->time_add  = time(NULL);
    path->is_used = true;
}

static bool is_this_path(router_path_t *path, const network_node_t *node_head,
                         const network_node_t *nodes, uint32_t nodes_cnt)
{
    if(path->nodes[0].addr_mac != node_head->addr_mac) { return false; }
    if(path->nodes[1].addr_mac != nodes[0].addr_mac) { return false; }
    return true;
}

void dev_router_add_path(const network_node_t *node_head,
                         const network_node_t *nodes, uint32_t nodes_cnt)
{
    int path_idx = 0;
    router_path_t *path = NULL;

    for (path_idx = 0; path_idx < ROUTER_PATH_MAX_CNT; ++path_idx) {
        path = &(g_paths[path_idx]);
        if(!path->is_used) { path = NULL;  continue; }

        if(is_this_path(path, node_head, nodes, nodes_cnt)) {
            break;
        }

        path = NULL;
    }

    if(!path) {
        for (path_idx = 0; path_idx < ROUTER_PATH_MAX_CNT; ++path_idx) {
            path = &(g_paths[path_idx]);
            if(!path->is_used) {
                break;
            } else {
                path = NULL;
            }
        }

    }

    if(path){
        set_path(path, node_head, nodes, nodes_cnt);
    } else {
        log_err("Router path is full!");
    }

}

static void cli_router(cli_cmd_t *cmd)
{
    int path_idx = 0;
    int node_idx = 0;
    router_path_t *path = NULL;
    network_node_t *node = NULL;
    for (path_idx = 0; path_idx < ROUTER_PATH_MAX_CNT; ++path_idx) {
        path = &(g_paths[path_idx]);
        if(!path->is_used) { continue; }
        cli_output(cmd, "PATH %d: ", path_idx);

        cli_output(cmd, "#->");
        for (node_idx = 0; node_idx < path->nodes_cnt; ++node_idx) {
            node = &(path->nodes[node_idx]);
            cli_output(cmd, "%s->", node->dev_name);
        }
        cli_output(cmd, "*\n");
    }
}

void dev_router_set_mac_local(addr_mac_t mac_local)
{
    g_mac_local = mac_local;
}

void dev_router_init(void)
{
    cli_regist("router", cli_router);
}
