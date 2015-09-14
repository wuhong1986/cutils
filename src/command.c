/* {{{
 * =============================================================================
 *      Filename    :   cmd.c
 *      Description :
 *      Created     :   2014-09-10 09:09:11
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include "command.h"
#include "ex_memory.h"
#include "ex_time.h"
#include "check_sum.h"
#include "cobj_cmd_routine.h"
#include "cobj_cmd_recv.h"
#include "dev_addr.h"
#include "cobj_addr.h"
#include "cthread.h"
#include "chash.h"
#include "clog.h"
#include "ccli.h"
#include "csem.h"
#include "ex_assert.h"
#include <errno.h>

#define CMD_START_BYTE      0xAA
#define CMD_START_BYTE_LEN  (1)

#define CMD_OT_CHECK_PERIOD (TIME_100MS)
#define CMD_OT_CHECK_TIME   (TIME_1S)
#define CMD_OT_DEFAULT      (2 * TIME_1S)
/* #define CMD_DEBUG_TX */
/* #define CMD_DEBUG_RX */

#define CMD_GET_BODY_TLVP(cmd) ((cmd)->body.tlvp)
#define CMD_GET_BODY_DATA(cmd) ((cmd)->body.data)
#define CMD_BODY_TYPE_IS_TLV(cmd)   ((cmd)->head.part_1.body_type == CMD_BODY_TYPE_TLV)
#define CMD_BODY_TYPE_IS_DATA(cmd)  ((cmd)->head.part_1.body_type == CMD_BODY_TYPE_DATA)
#define CMD_GET_HEAD_OPS(cmd)   (g_cmd_head_ops[cmd_get_type(cmd)])
#define CMD_GET_HEAD_PART2(cmd) (&(cmd->head.part_2))


static cmd_callback_set_dev_addr g_cb_set_dev_addr = NULL;
static cmd_head_ops_t g_cmd_head_ops[ADDR_TYPE_MAX_CNT];
/* ==========================================================================
 *        全局变量
 * ======================================================================= */
static chash *g_routine_list_req  = NULL;
static chash *g_routine_list_resp = NULL;
static cmd_routine_t *g_routine_req_default = NULL;
static cmd_routine_t *g_routine_resp_default = NULL;

/* =============================================================================
 *  请求命令相关
 * ========================================================================== */
static  csem* g_sem_ot_cnt = NULL;

static  clist *g_cmd_list_req = NULL;    /* 请求命令列表 */
static  clist *g_cmd_list_ot  = NULL;    /* 超时命令列表 */

/* =============================================================================
 *  接收命令相关
 * ========================================================================== */
static csem  *g_sem_recv_cnt  = NULL;
static csem  *g_sem_recv = NULL;
static clist *g_cmd_list_recv[CMD_PRIOR_CNT];

static cmd_req_t* cmd_request_list_pop(const cmd_t *cmd_resp);

static uint32_t cmd_head_common_get_size(const void *head2)
{
    return 0;
}

void cmd_head_set_ops(uint8_t head_type, const cmd_head_ops_t *ops)
{
    g_cmd_head_ops[head_type] = *ops;
}

inline static cmd_type_t cmd_get_type(const cmd_t *cmd)
{
    return cmd->head.part_1.cmd_type;
}

inline static void cmd_set_type(cmd_t *cmd, cmd_type_t cmd_type)
{
    cmd->head.part_1.cmd_type = cmd_type;
}

bool cmd_is_resp(const cmd_t *cmd)
{
    return cmd->head.part_1.cmd_code & 0x8000;
}

inline static void cmd_set_is_resp(cmd_t *cmd, bool is_resp)
{
    is_resp
  ? (cmd->head.part_1.cmd_code |= 0x8000)
  : (cmd->head.part_1.cmd_code &= 0x7FFF);
}

void cmd_req_set_async(cmd_req_t *cmd_req)
{
    cmd_req->cmd.head.part_1.req_type = CMD_REQ_TYPE_ASYNC;
}

void cmd_req_set_sync(cmd_req_t *cmd_req)
{
    cmd_req->cmd.head.part_1.req_type = CMD_REQ_TYPE_SYNC;
}

void cmd_set_req_type(cmd_t *cmd, cmd_req_type_t req_type)
{
    cmd->head.part_1.req_type = req_type;
}

cmd_req_type_t cmd_get_req_type(cmd_t *cmd)
{
    return cmd->head.part_1.req_type;
}

void cmd_set_dev_addr(cmd_callback_set_dev_addr cb)
{
    g_cb_set_dev_addr = cb;
}

static void cmd_obj_release(cmd_t *cmd)
{
    tlvp_t  *tlvp = NULL;
    uint8_t *data = NULL;

    if(CMD_BODY_TYPE_IS_TLV(cmd)){
        tlvp = CMD_GET_BODY_TLVP(cmd);
        if(tlvp){ tlvp_free(tlvp); }
    } else if(CMD_BODY_TYPE_IS_DATA(cmd)) {
        data = CMD_GET_BODY_DATA(cmd);
        if(data) { free(data); }
    }
}

void cmd_free(cmd_t *cmd)
{
    cmd_obj_release(cmd);
    free(cmd);
}

static void cobj_cmd_req_free(void *val)
{
    cmd_req_t *req = (cmd_req_t*)val;
    cmd_obj_release(&(req->cmd));
    if(req->sem_sync){
        csem_free(req->sem_sync);
    }
    if(req->cmd_sync_recv){
        cobj_free(req->cmd_sync_recv);
    }
}

static inline void cmd_req_do_free(cmd_req_t *cmd_req)
{
    cobj_free(cmd_req);
}

void cmd_req_free(cmd_req_t *cmd_req)
{
    /* free otherwher */

    ex_assert(CMD_REQ_TYPE_SYNC == cmd_get_req_type(&(cmd_req->cmd)));
    /* cmd_req_t  */
    if(CMD_REQ_TYPE_SYNC == cmd_get_req_type(&(cmd_req->cmd))){
        cmd_req_do_free(cmd_req);
    }
}

static cobj_ops_t cobj_ops_cmd_req = {
    .name = "CmdReq",
    .obj_size = sizeof(cmd_req_t),
    .cb_destructor = cobj_cmd_req_free,
};

inline static void cmd_list_req_lock(void)
{
    clist_lock(g_cmd_list_req);
}

inline static void cmd_list_req_unlock(void)
{
    clist_unlock(g_cmd_list_req);
}

inline static void cmd_list_ot_lock(void)
{
    clist_lock(g_cmd_list_ot);
}

inline static void cmd_list_ot_unlock(void)
{
    clist_unlock(g_cmd_list_ot);
}

cmd_code_t cmd_get_code(const cmd_t *cmd)
{
    return cmd->head.part_1.cmd_code & 0x7FFF;
}

void cmd_set_code(cmd_t *cmd, cmd_code_t code, bool is_resp)
{
    is_resp
  ? (cmd->head.part_1.cmd_code = (code | 0x8000))
  : (cmd->head.part_1.cmd_code = (code & 0x7FFF));
}

cmd_idx_t cmd_get_idx(const cmd_t *cmd)
{
    return cmd->head.part_1.cmd_idx;
}

void cmd_set_idx(cmd_t *cmd, cmd_idx_t idx)
{
    cmd->head.part_1.cmd_idx = idx;
}

void cmd_set_addr_net_src(cmd_t *cmd, addr_net_t addr_net_src)
{
    cmd->head.part_1.addr_net_src = addr_net_src;
}

void cmd_set_addr_net_dst(cmd_t *cmd, addr_net_t addr_net_dst)
{
    cmd->head.part_1.addr_net_dst = addr_net_dst;
}

void cmd_set_addr_mac_src(cmd_t *cmd, addr_mac_t addr_mac_src)
{
    cmd->head.part_1.addr_mac_src = addr_mac_src;
}

void cmd_set_addr_mac_dst(cmd_t *cmd, addr_mac_t addr_mac_dst)
{
    cmd->head.part_1.addr_mac_dst = addr_mac_dst;
}

addr_net_t cmd_get_addr_net_src(cmd_t *cmd)
{
    return cmd->head.part_1.addr_net_src;
}

addr_net_t cmd_get_addr_net_dst(cmd_t *cmd)
{
    return cmd->head.part_1.addr_net_dst;
}

addr_mac_t cmd_get_addr_mac_src(cmd_t *cmd)
{
    return cmd->head.part_1.addr_mac_src;
}

addr_mac_t cmd_get_addr_mac_dst(cmd_t *cmd)
{
    return cmd->head.part_1.addr_mac_dst;
}

cmd_body_type_t cmd_get_body_type(const cmd_t *cmd)
{
    return cmd->head.part_1.body_type;
}

void cmd_set_body_type(cmd_t *cmd, cmd_body_type_t type)
{
    cmd->head.part_1.body_type = type;
}

uint32_t cmd_get_data_len(const cmd_t *cmd)
{
    return cmd->head.part_1.data_len;
}

inline static void cmd_set_data_len(cmd_t *cmd, uint16_t len)
{
    cmd->head.part_1.data_len = len;
}

inline static uint32_t cmd_get_head_size(const cmd_t *cmd)
{
    return sizeof(cmd_head_1_t) + CMD_GET_HEAD_OPS(cmd).cb_get_size(cmd) + 1;
}

inline static uint32_t cmd_get_mac_src(const cmd_t *cmd)
{
    return CMD_GET_HEAD_OPS(cmd).cb_get_mac_src
         ? CMD_GET_HEAD_OPS(cmd).cb_get_mac_src(CMD_GET_HEAD_PART2(cmd))
         : 0xFFFFFFFF;
}

inline static uint32_t cmd_get_mac_desc(const cmd_t *cmd)
{
    return CMD_GET_HEAD_OPS(cmd).cb_get_mac_desc
         ? CMD_GET_HEAD_OPS(cmd).cb_get_mac_desc(CMD_GET_HEAD_PART2(cmd))
         : 0xFFFFFFFF;
}

cmd_prior_t cmd_get_prior(const cmd_t *cmd)
{
    return cmd->head.part_1.prior;
}

void cmd_set_prior(cmd_t *cmd, cmd_prior_t prior)
{
    cmd->head.part_1.prior = prior;
}

void cmd_set_error(cmd_t *cmd, cmd_error_t error)
{
    cmd->head.part_1.error = error;
}

cmd_error_t cmd_get_error(const cmd_t *cmd)
{
    return cmd->head.part_1.error;
}

const char *cmd_get_dev_addr_name(const cmd_t *cmd)
{
    return dev_addr_get_name(cmd->dev_addr);
}

/**
 * @Brief  在命令处理回调函数列表中查找对应命令码对应的处理函数
 *
 * @Param list      命令函数列表
 * @Param cmd_code  命令码
 *
 * @Returns   NULL： 没有该命令码对应的处理函数
 *            其他值： 对应的命令处理函数
 */

static  cmd_routine_t *find_cmd_routine(chash *hash, uint32_t cmd_code,
                                        cmd_routine_t *routine_default)
{
    cmd_routine_t *routine = NULL;

    routine = (cmd_routine_t*)chash_int_get(hash, cmd_code);
    if(!routine) { routine = routine_default; }

    return routine;
}

static  cmd_routine_t *find_cmd_routine_req(uint32_t cmd_code)
{
    return find_cmd_routine(g_routine_list_req, cmd_code, g_routine_req_default);
}

static  cmd_routine_t *find_cmd_routine_resp(uint32_t cmd_code)
{
    return find_cmd_routine(g_routine_list_resp, cmd_code, g_routine_resp_default);
}

/**
 * @Brief
 *
 * @Param list
 * @Param cmd_code
 * @Param func
 *
 * @Returns
 */
static cmd_routine_t* cmd_routine_regist(chash *hash, uint32_t cmd_code,
                                         cmd_callback_req  cb_req,
                                         cmd_callback_ot   cb_ot,
                                         cmd_callback_resp cb_resp)
{
    cmd_routine_t *routine = NULL;

    routine = find_cmd_routine(hash, cmd_code, NULL);
    if(routine) {
        log_warn("Cmd code:%d already exist proc func!", cmd_code);
        /* assert_ex(0); */
        exit(-1);
    }

    routine = cobj_cmd_routine_new(cmd_code, cb_req, cb_ot, cb_resp);

    chash_int_set(hash, cmd_code, routine);

    return routine;
}

void cmd_routine_regist_resp(uint32_t cmd_code,
                             cmd_callback_resp cb_resp,
                             cmd_callback_ot   cb_ot)
{
    cmd_routine_t *routine = NULL;

    routine = cmd_routine_regist(g_routine_list_resp, cmd_code, NULL, cb_ot, cb_resp);

    if(cmd_code == CMD_CODE_DEFAULT) {
        g_routine_resp_default = routine;
    }
}

void cmd_routine_regist_req(uint32_t cmd_code, cmd_callback_req cb_req)
{
    cmd_routine_t *routine = NULL;

    cmd_routine_regist(g_routine_list_req, cmd_code, cb_req, NULL, NULL);

    if(cmd_code == CMD_CODE_DEFAULT) {
        g_routine_req_default = routine;
    }
}

cmd_req_t *cmd_new_req(dev_addr_t *dev_addr, cmd_code_t code)
{
    cmd_req_t *cmd_req = ex_malloc_one(cmd_req_t);
    cmd_t *cmd = &(cmd_req->cmd);

    cobj_set_ops(cmd_req, &cobj_ops_cmd_req);

    cmd_req->time_ot   = CMD_OT_DEFAULT;
    cmd_req->time_left = CMD_OT_DEFAULT;
    cmd_req->time_req  = time(NULL);
    cmd_req->time_sent = 0;
    cmd_req->is_sent   = false;

    cmd->dev_addr = dev_addr;
    cmd_set_code(cmd, code, false);
    cmd_set_idx(cmd, dev_addr_get_next_cmd_idx(dev_addr));

    return cmd_req;
}

cmd_t *cmd_new_resp(cmd_t *cmd_req)
{
    cmd_t *cmd = NULL;

    cmd = ex_malloc_one(cmd_t);
    cmd->dev_addr = cmd_req->dev_addr;

    cmd_set_type(cmd, cmd_get_type(cmd_req));
    cmd_set_code(cmd, cmd_get_code(cmd_req), true);
    cmd_set_idx(cmd, cmd_get_idx(cmd_req));
    cmd_set_req_type(cmd, cmd_get_req_type(cmd_req));
    /* cmd_head_init_resp(&(cmd->head), &(cmd_req->head)); */
    /* printf("cmd get idx:%d\n", cmd_get_idx(cmd_req)); */

    return cmd;
}

void   cmd_set_resp_post(cmd_t *cmd_resp, cmd_callback_resp_post cb)
{
    cmd_resp->cb_resp_post = cb;
}

static bool cmd_equal(const cmd_t *cmd1, const cmd_t *cmd2)
{
    /* log_dbg("cmd1: %d-%d cmd2:%d-%d", */
    /*         cmd_get_idx(cmd1), cmd_get_code(cmd1), */
    /*         cmd_get_idx(cmd2), cmd_get_code(cmd2) */
    /*         ); */
    if(   cmd1->dev_addr == cmd2->dev_addr
       && cmd_get_idx(cmd1)  == cmd_get_idx(cmd2)
       && cmd_get_code(cmd1) == cmd_get_code(cmd2)) {
        /* log_dbg("equal"); */
        return true;
    }

    return false;
}

/**
 * @Brief  请求列表中删除命令
 *
 * @Param cmd
 *
 * @Returns   ERR_CMD_NO_SUCH_REQ_CMD: 不存在该请求命令，或者已经进行超时处理了
 */
static cmd_req_t* cmd_request_list_pop(const cmd_t *cmd_resp)
{
    cmd_req_t *req   = NULL;
    bool      bExist = false;
    Status    ret    = S_OK;
    clist_iter iter;

    cmd_list_req_lock();

    iter = clist_begin(g_cmd_list_req);
    clist_iter_foreach_obj(&iter, req){
        if(cmd_equal(&(req->cmd), cmd_resp)){
            clist_pop(&iter);
            break;
        }

        req = NULL;
    }

    cmd_list_req_unlock();

    return req;
}

cmd_t* cmd_recv_sync_response(cmd_req_t *req)
{
    if(csem_down_timed(req->sem_sync, req->time_ot) == 0) {
        return req->cmd_sync_recv->cmd;
    } else {
        return NULL;
    }
}

static Status cmd_process_recv_resp_sync(cmd_recv_t *cmd_recv)
{
    cmd_req_t *req = NULL;
    cmd_t     *cmd = cmd_recv->cmd;
    clist_iter iter;

    cmd_list_req_lock();

    /* log_dbg("list size:%d", clist_size(g_cmd_list_req)); */

    iter = clist_begin(g_cmd_list_req);
    clist_iter_foreach_obj(&iter, req){
        if(cmd_equal(&(req->cmd), cmd)){
            clist_pop(&iter);
            break;
        }
    }

    if(NULL == req) {
        log_dbg("recv sync response, can't find request");
        cobj_free(cmd_recv);
    } else {
        req->cmd_sync_recv = cmd_recv;
        csem_up(req->sem_sync);
    }

    cmd_list_req_unlock();

    return S_OK;
}

static Status cmd_process_recv_resp_async(cmd_recv_t *cmd_recv)/*{{{*/
{
    cmd_routine_t *routine = NULL;
    Status        ret      = S_OK;
    uint32_t      cmd_code = 0;
    cmd_t         *cmd     = NULL;
    cmd_req_t     *cmd_req = NULL;

    cmd = cmd_recv->cmd;
    cmd_code = cmd_get_code(cmd);

    /* log_dbg("Recv resp cmd, dev:%s code:%d, cmd_id:%d", */
    /*         dev_addr_get_name(cmd->dev_addr), cmd_code, cmd_get_id(cmd)); */

    /*  接收到应答, 从管理链表中删除之前的请求命令 */
    cmd_req = cmd_request_list_pop(cmd);
    if(!cmd_req){
        /*  1. 该请求已经超时了，在请求列表中没有找到请求，不处理, 丢弃该应答命令
         *  2. 数据应答方对数据进行重发，重复接收，之前已经删除了该请求命令
         *  3. 数据请求方对请求进行重发，对方重复应答
         * */
        log_dbg("discard resp cmd, dev:%s code:%d, cmd_idx:%d",
                cmd_get_dev_addr_name(cmd), cmd_get_code(cmd), cmd_get_idx(cmd));
        return ret;
    }

    routine = find_cmd_routine_resp(cmd_code);

    if(NULL == routine){
        log_dbg("cmd code:%d can't find process routine!", cmd_code);
        ret = ERR_CMD_ROUTINE_NOT_FOUND; /*  没有注册该命令 */
    } else if(routine->callback_resp) {
        ret = routine->callback_resp(cmd_req, cmd);
    }

    cmd_req_do_free(cmd_req);

    return ret;
}/*}}}*/

static Status cmd_process_recv_resp(cmd_recv_t *cmd_recv)/*{{{*/
{
    if(CMD_REQ_TYPE_ASYNC == cmd_get_req_type(cmd_recv->cmd)) {
        return cmd_process_recv_resp_async(cmd_recv);
    } else {
        return cmd_process_recv_resp_sync(cmd_recv);
    }
}/*}}}*/

static Status cmd_process_recv_req(cmd_recv_t *cmd_recv)/*{{{*/
{
    cmd_routine_t *routine  = NULL;
    Status        ret       = S_OK;
    uint32_t      cmd_code  = 0;
    cmd_t         *cmd      = NULL;
    cmd_t         *cmd_resp = NULL;

    cmd = cmd_recv->cmd;
    cmd_code = cmd_get_code(cmd);

    /*  接收到远程的请求命令 */
    routine = find_cmd_routine_req(cmd_code);
    /* log_dbg("recv req code:%d idx:%d", cmd_code, cmd_get_idx(cmd)); */

    cmd_resp = cmd_new_resp(cmd);

    if(NULL == routine){
        log_err("Cmd Code:%d idx:%d Routine Not Found", cmd_code, cmd_get_idx(cmd));
        cmd_set_error(cmd_resp, CMD_E_NO_ROUTINE); /*  没有注册该命令，返回错误信息 */
    } else if(routine->callback_req){
        routine->callback_req(cmd, cmd_resp);
    }

    /* 如果有地址信息，需要添加 */
    if(g_cb_set_dev_addr) {
        g_cb_set_dev_addr(cmd_resp);
    }

    ret = cmd_send_resp(cmd_resp);

    if(S_OK == ret && cmd_resp->cb_resp_post) {
        cmd_resp->cb_resp_post(cmd, cmd_resp);
    }

    cmd_free(cmd_resp);

    return ret;
}/*}}}*/

static Status cmd_process_recv(cmd_recv_t *cmd_recv)/*{{{*/
{
    if(cmd_is_resp(cmd_recv->cmd)){
        return cmd_process_recv_resp(cmd_recv);
    } else {
        return cmd_process_recv_req(cmd_recv);
    }
}/*}}}*/

static Status cmd_transfer_recv(cmd_recv_t *cmd_recv)
{
    log_dbg("transfer msg");
    /* 修改 cmd 发送目的地 */
    return cmd_send(cmd_recv->cmd);
}

void cmd_param_finish(cmd_t *cmd)
{
    uint32_t data_len = 0;

    if(CMD_BODY_TYPE_TLV == cmd_get_body_type(cmd)){
        tlvp_t *tlvp = CMD_GET_BODY_TLVP(cmd);
        if(NULL == tlvp || tlvp_cnt(tlvp) == 0) data_len = 0;
        else data_len = tlvp_sizeof(tlvp);
        cmd_set_data_len(cmd, data_len);
    }

    cmd->head.check_sum = check_sum_u8(&(cmd->head), sizeof(cmd_head_t) - 1);
}

uint32_t cmd_sizeof(const cmd_t *cmd)
{
    uint32_t cmd_size = 0;
    uint32_t data_len = cmd_get_data_len(cmd);

    cmd_size = CMD_START_BYTE_LEN      /*  起始字符 */
             + cmd_get_head_size(cmd)  /*  头部信息 */
             + data_len;       /*  数据 */
    if(data_len > 0) { cmd_size += 1; } /* 有数据，需加一位数据校验码 */

    return cmd_size;
}

void cmd_to_data(const cmd_t *cmd, uint8_t *data, uint32_t data_len)
{
    uint8_t   *data_body = NULL;
    uint32_t   body_len  = 0;
    uint32_t   head_len  = 0;
    uint32_t  offset     = 0;

    UNUSED(data_len);

    data[0] = CMD_START_BYTE;
    offset += CMD_START_BYTE_LEN;

    head_len = cmd_get_head_size(cmd);
    memcpy(&(data[offset]), &(cmd->head.part_1), sizeof(cmd_head_1_t));
    if(CMD_GET_HEAD_OPS(cmd).cb_copy){
        CMD_GET_HEAD_OPS(cmd).cb_copy(&(cmd->head.part_2),
                                      &(data[offset + sizeof(cmd_head_1_t)]),
                                      head_len - sizeof(cmd_head_1_t));
    }
    data[offset + head_len - 1] = cmd->head.check_sum;
    /* printf("head check sum: %02X", cmd->head.check_sum); */
    offset += head_len;

    data_body = &(data[offset]);
    body_len = cmd_get_data_len(cmd);
    if(body_len > 0){
        if(CMD_BODY_TYPE_IS_TLV(cmd)) {
            tlvp_to_data(CMD_GET_BODY_TLVP(cmd), data_body, body_len);
        } else if(CMD_BODY_TYPE_IS_DATA(cmd)) {
            memcpy(data_body, CMD_GET_BODY_DATA(cmd), body_len);
        }

        /*  计算数据校验码 */
        offset += body_len;
        data[offset] = check_sum_u8(data_body, body_len);
    }
}

/**
 * @Brief  发送命令
 *         首先组装命令，包括起始字符，头部和数据以及校验码
 *
 * @Param addr
 * @Param addrType
 * @Param cmd_head
 * @Param tlv_pack
 *
 * @Returns
 */
Status cmd_send(cmd_t *cmd)
{
    Status   ret      = S_OK;
    uint8_t  *data    = NULL;
    uint32_t data_len = 0;
    int      nSend    = 0;
    dev_addr_t *dev_addr_tx = NULL;
#ifdef CMD_DEBUG_TX
    uint32_t i = 0;
#endif

    cmd_param_finish(cmd);

    data_len = cmd_sizeof(cmd);
    data     = ex_malloc(uint8_t, data_len);

    /* log_dbg("data_len:%d", data_len); */

    cmd_to_data(cmd, data, data_len);

    dev_addr_tx = cmd->dev_addr;
    if(dev_addr_tx->is_endpoint) {
        /* log_dbg("Transfer msg from %s to %s", */
        /*             dev_addr_get_name(dev_addr_tx->addr_parent), */
        /*             dev_addr_get_name(dev_addr_tx)); */
        dev_addr_tx = dev_addr_tx->addr_parent;
    }

    if(dev_addr_tx->is_remote) {
        /* log_dbg("Transfer msg from %s to %s", */
        /*             dev_addr_get_name(dev_addr_tx->addr_router), */
        /*             dev_addr_get_name(dev_addr_tx)); */
        dev_addr_tx = dev_addr_tx->addr_router;
    }

    nSend = dev_addr_send(dev_addr_tx, data, data_len);
    if(nSend != (int)data_len){
        log_dbg("cmd send data failed: %d", nSend);
        if(nSend < 0) {
            ret = -nSend;
        } else {
            ret = ERR_CMD_SEND_FAILED;
        }
    }

#ifdef CMD_DEBUG_TX
    printf("Tx:0x");
    for (i = 0; i < data_len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
#endif

    ex_free(data);

    return ret;
}

inline static void cmd_req_list_add(cmd_req_t *cmd_req)
{
    cmd_list_req_lock();

    /* log_dbg("cmd req list add node"); */
    clist_append(g_cmd_list_req, cmd_req);

    cmd_list_req_unlock();
}

inline static void cmd_req_node_set_sent(cmd_req_t *req)
{
#if 0
    cmd_req_t *req_find = NULL;

    cmd_list_req_lock();

    /* TODO: 修改为逆向查找 */
    req_find = (cmd_req_t*)clist_find_obj(g_cmd_list_req, req);
    if(NULL != req_find){
        req_find->is_sent = true;
    }

    cmd_list_req_unlock();
#else
    req->is_sent = true;
#endif
}

inline static void cmd_req_node_delete(cmd_req_t *req)
{
    cmd_list_req_lock();

    clist_find_then_remove(g_cmd_list_req, req);

    cmd_list_req_unlock();
}


static Status cmd_send_request_async(cmd_t *cmd)
{
    return S_OK;
}

static Status cmd_send_request_sync(cmd_t *cmd)
{
    return S_OK;
}

/**
 * @Brief  发送请求命令
 *
 * @Param cmd
 *
 * @Returns
 */
Status cmd_send_request(cmd_req_t *req)
{
    Status ret          = S_OK;
    bool   is_need_resp = true;
    cmd_t  *cmd         = &(req->cmd);
    addr_net_t addr_net_src = 0;
    addr_net_t addr_net_dst = 0;
    addr_mac_t addr_mac_src = 0;
    addr_mac_t addr_mac_dst = 0;

    if(req->time_ot == CMD_REQ_TIMEOUT_NO_RESP) is_need_resp = false;

    if(CMD_REQ_TYPE_SYNC == cmd_get_req_type(cmd)) {
        req->sem_sync = csem_new(0);
        is_need_resp = true;
    }

    if(is_need_resp) {
        cmd_req_list_add(req);
    }

    addr_mac_dst = cmd->dev_addr->addr_mac;
    addr_net_dst = cmd->dev_addr->addr_net;
    addr_mac_src = dev_addr_mgr_get_addr_mac();
    if(cmd->dev_addr->is_endpoint) {
        addr_mac_dst = cmd->dev_addr->addr_parent->addr_mac;
    }

    cmd_set_addr_net_src(cmd, addr_net_src);
    cmd_set_addr_net_dst(cmd, addr_net_dst);
    cmd_set_addr_mac_src(cmd, addr_mac_src);
    cmd_set_addr_mac_dst(cmd, addr_mac_dst);

    ret = cmd_send(cmd);
    if(ret == S_OK && is_need_resp) {
        cmd_req_node_set_sent(req);
    } else {
        if(!is_need_resp){
            cmd_req_do_free(req);
        } else if(CMD_REQ_TYPE_SYNC != cmd_get_req_type(cmd)){
            cmd_req_node_delete(req);
        }
    }

    return ret;
}

Status cmd_send_resp(cmd_t *cmd)
{
    return cmd_send(cmd);
}

/* =============================================================================
 *  接收命令相关
 * ========================================================================== */

void  cmd_recv_list_add(cmd_t *cmd, uint8_t *data, uint32_t data_len)
{
    cmd_recv_t *cmd_recv = NULL;
    cmd_prior_t prior;

    cmd_recv = cobj_cmd_recv_new(cmd, data, data_len);

    cmutex_lock(g_sem_recv);

    prior = cmd_get_prior(cmd);
    if(prior > CMD_PRIOR_CNT) {
        log_err("cmd prior %d error, max is:%d, code:%d",
                prior, CMD_PRIOR_CNT, cmd_get_code(cmd));
        ex_assert(0);
        prior = CMD_PRIOR_MID;
    }

    clist_append(g_cmd_list_recv[prior], cmd_recv);

    csem_up(g_sem_recv_cnt);
    cmutex_unlock(g_sem_recv);
}

inline static cmd_recv_status_t recv_buf_get_status(const cmd_recv_buf_t *recv_buf)
{
    return recv_buf->recv_status;
}

inline static void recv_buf_set_status(cmd_recv_buf_t *recv_buf,
                                       cmd_recv_status_t status)
{
    recv_buf->recv_status = status;
}

void cmd_receive(addr_t *addr, const void *data, uint32_t len)
{
    cmd_recv_buf_t *recv_buf = addr_get_recv_buf(addr);

    uint8_t    start_byte = 0;
    uint8_t    size_head  = 0;
    uint32_t   buf_left   = len;
    uint32_t   len_read   = 0;
    uint8_t    check_sum  = 0;
    uint32_t   data_recv  = 0;
    cmd_t      *cmd       = NULL;
    cmd_head_t *head      = NULL;
#ifdef CMD_DEBUG_RX
    uint32_t  i = 0;
    printf("Rx:0x");
    for (i = 0; i < len; i++) {
        printf("%02X ", ((uint8_t*)data)[i]);
    }
    printf("\n");
#endif

    /* printf("cmd recv data, length:%d\n", len); */

    addr_lock_recv(addr);

    head = &(recv_buf->recv_head);
restart_recv:
    if(CMD_RECV_S_START_BYTE == recv_buf_get_status(recv_buf)) {
        len_read = 1;
        while(buf_left >= len_read) {
            memcpy(&start_byte, (uint8_t*)data + len - buf_left, len_read);
            start_byte = *((uint8_t*)data + len - buf_left);
            --buf_left;
            if(start_byte == CMD_START_BYTE){
                /* LoggerDebug("start byte OK!"); */
                recv_buf_set_status(recv_buf, CMD_RECV_S_HEAD_1);
                break;
            } else {
                log_dbg("start byte error, we want:0x%02X but recv:0x%02X",
                        CMD_START_BYTE, start_byte);
                /* uint32_t  i = 0; */
                /* printf("Rx:0x"); */
                /* for (i = 0; i < len; i++) { */
                /*     printf("%02X ", ((uint8_t*)data)[i]); */
                /* } */
                /* printf("\n"); */
                /* exit(0); */
            }
        }

        /* reset cmd head */
        recv_buf->recv_head_len_1 = 0;
        ex_memzero_one(head);
    }

    if(CMD_RECV_S_HEAD_1 == recv_buf_get_status(recv_buf)) {
        size_head = sizeof(cmd_head_1_t);
        len_read = size_head - recv_buf->recv_head_len_1;
        if(len_read > buf_left) len_read = buf_left;
        if(len_read > 0) {
            memcpy(&(((uint8_t*)(&(head->part_1)))[recv_buf->recv_head_len_1]),
                   (uint8_t*)data + len - buf_left,
                   len_read);
            buf_left -= len_read;
            recv_buf->recv_head_len_1 += len_read;
        }

        if(recv_buf->recv_head_len_1 >= size_head) {
            recv_buf_set_status(recv_buf, CMD_RECV_S_HEAD_2);
            recv_buf->recv_head_len_2 = 0;
        }
    }

    if(CMD_RECV_S_HEAD_2 == recv_buf_get_status(recv_buf)) {
        /* printf("recv head2--0"); */
        if(g_cmd_head_ops[head->part_1.cmd_type].cb_get_size){
            size_head = g_cmd_head_ops[head->part_1.cmd_type].cb_get_size(NULL);
            /* printf("recv head2--1, size head:%d", size_head); */

            len_read = size_head - recv_buf->recv_head_len_2;
            if(len_read > buf_left) len_read = buf_left;
            if(len_read > 0) {
                /* printf("recv head2--2"); */
                memcpy(&(head->part_2[recv_buf->recv_head_len_2]),
                       (uint8_t*)data + len - buf_left,
                       len_read);
                buf_left -= len_read;
                recv_buf->recv_head_len_2 += len_read;
            }
        } else {
            /* printf("recv head2--3"); */
            size_head = 0;
        }

        if(recv_buf->recv_head_len_2 >= size_head) {
            recv_buf_set_status(recv_buf, CMD_RECV_S_HEAD_CHECKSUM);
        }
    }

    if(CMD_RECV_S_HEAD_CHECKSUM == recv_buf_get_status(recv_buf)) {
        len_read = 1;
        if(len_read > buf_left) len_read = buf_left;
        if(len_read > 0) {
            memcpy(&(head->check_sum), (uint8_t*)data + len - buf_left, len_read);
            buf_left -= len_read;

            size_head = sizeof(cmd_head_1_t)
                      + g_cmd_head_ops[head->part_1.cmd_type].cb_get_size(NULL);

            /* LoggerDebug("recv cmd head"); */
            /* bufferevent_read(bev, &(pAddr->addr_sock.head), sizeof(CmdHead_T)); */
            check_sum = check_sum_u8(head, size_head);
            if(check_sum != head->check_sum){
                log_warn("Cmd Check failed, head size:%d buf left:%d "
                         " we want:0x%02X, but result is 0x%02X",
                         size_head, buf_left, check_sum, head->check_sum);
                /* exit(0); */
                recv_buf_set_status(recv_buf, CMD_RECV_S_START_BYTE);
                goto restart_recv;
            } else { /* 校验成功 */
                recv_buf_set_status(recv_buf, CMD_RECV_S_BODY_START);
            }
        }
    }

    if(CMD_RECV_S_BODY_START == recv_buf_get_status(recv_buf)) {
        if(head->part_1.data_len > 0){
            recv_buf->recv_data_len    = head->part_1.data_len + 1; /* 加一个校验位 */
            recv_buf->recv_data_offset = 0;
            recv_buf->recv_data        = ex_malloc(uint8_t, recv_buf->recv_data_len);
            recv_buf_set_status(recv_buf, CMD_RECV_S_BODY);
        } else {
            /* 没有数据，接收成功 */
            recv_buf->recv_data_len    = 0; /* 加一个校验位 */
            recv_buf->recv_data_offset = 0;
            recv_buf->recv_data        = NULL;
            recv_buf_set_status(recv_buf, CMD_RECV_S_FINISHED);
        }
    }

    if(CMD_RECV_S_BODY == recv_buf_get_status(recv_buf)){
        /* 需要判断是分次接收，还是一次性全部接收 */
        data_recv = recv_buf->recv_data_len - recv_buf->recv_data_offset;
        if(buf_left < data_recv) { data_recv = buf_left; }

        memcpy(recv_buf->recv_data + recv_buf->recv_data_offset,
               (uint8_t*)data + len - buf_left,
               data_recv);
        recv_buf->recv_data_offset += data_recv;
        buf_left -= data_recv;

        /* LoggerDebug("max read:%d recv:%d progress %d/%d.", */
        /*             bufferevent_get_max_to_read(bev), data_recv, */
        /*             addr_sock->data_recv, addr_sock->data_len); */

        if(recv_buf->recv_data_offset >= recv_buf->recv_data_len){
            check_sum = check_sum_u8(recv_buf->recv_data,
                                     recv_buf->recv_data_len - 1); /* 校验码自身不校验 */
            if(check_sum != recv_buf->recv_data[recv_buf->recv_data_len - 1]){
                log_warn("Cmd Body Check failed, we want:0x%02X, "
                           "but result is 0x%02X",
                           check_sum, recv_buf->recv_data[recv_buf->recv_data_len - 1]);
                ex_free(recv_buf->recv_data);
                recv_buf_set_status(recv_buf, CMD_RECV_S_START_BYTE);
            } else {    /* 校验正确 */
                recv_buf_set_status(recv_buf, CMD_RECV_S_FINISHED);
            }
        }
    }

    if(CMD_RECV_S_FINISHED == recv_buf_get_status(recv_buf)) {
        cmd = ex_malloc_one(cmd_t);
        cmd->dev_addr = addr_get_dev_addr(addr);
        cmd->head     = *head;

        /*  转换为tlv 格式 */
        if(cmd_get_data_len(cmd) > 0){
            if(CMD_BODY_TYPE_IS_TLV(cmd)) {
                /*  转化成TLV格式数据 */
                cmd->body.tlvp = tlvp_new();
                tlvp_from_data(cmd->body.tlvp, recv_buf->recv_data, cmd_get_data_len(cmd));
                /*TlvPackPrint(tlv_pack);*/
            } else if(CMD_BODY_TYPE_IS_DATA(cmd)) {
                cmd->body.data = recv_buf->recv_data;
            }
        }

        cmd_recv_list_add(cmd, recv_buf->recv_data, recv_buf->recv_data_len);
        recv_buf_set_status(recv_buf, CMD_RECV_S_START_BYTE);
    }

    if(buf_left > 0 && CMD_RECV_S_START_BYTE == recv_buf_get_status(recv_buf)) {
        /* printf("continue to recv msg, buf left:%d\n", buf_left); */
        goto restart_recv;  /* 如果缓存中还有数据则继续接收 */
    }

    /* log_dbg("cmd rx end"); */
    addr_unlock_recv(addr);
}

static Status   cmd_recv_proc_thread_fun(const thread_t *thread)
{
    cmd_recv_t *cmd_recv = NULL;
    cmd_t      *cmd = NULL;
    clist      *cmd_list = NULL;
    uint8_t i = 0;
    cmd_req_type_t req_type;
    bool is_resp = false;
    bool is_transfer = false;

    UNUSED(thread);

    while (!thread_is_quit()) {
        if(csem_down_timed(g_sem_recv_cnt, TIME_100MS) != 0) { continue; }

        /* log_dbg("recv cmd"); */
        cmutex_lock(g_sem_recv);

        for(i = 0; i < CMD_PRIOR_CNT; ++i) {
            if(clist_size(g_cmd_list_recv[i]) > 0) {
                /* log_dbg("proc recv prior:%d", i); */
                cmd_list = g_cmd_list_recv[i];
                break;
            }
        }

        cmd_recv = (cmd_recv_t*)clist_pop_front(cmd_list);

        cmutex_unlock(g_sem_recv);

        cmd = cmd_recv->cmd;
        req_type = cmd_get_req_type(cmd);
        is_resp  = cmd_is_resp(cmd);
        /* is_transfer = (cmd_get_addr_mac_dst(cmd) != dev_addr_mgr_get_addr_mac()); */

        /* log_dbg("local mac:%08X dest mac:%08X", dev_addr_mgr_get_addr_mac(), */
        /*         cmd_get_addr_mac_dst(cmd)); */
        if(!is_transfer){
            cmd_process_recv(cmd_recv);
        } else {
            cmd_transfer_recv(cmd_recv);
        }

        if(is_transfer || CMD_REQ_TYPE_ASYNC == req_type || !is_resp){
            cobj_free(cmd_recv);
        }
    }

    return S_OK;
}

/**
 * @Brief  超时检测线程的入口函数
 *         检查哪些命令已经超时，
 *         并且把超时的命令挪动超时命令列表，
 *         同时唤醒超时处理线程处理超时命令
 *
 *         分成超时检测和超时处理2个线程的原因是防止超时处理函数处理时间过长，
 *         从而超时判断不准确，并且长时间占用g_cmd_list_req,
 *         导致发送线程不能及时发送命令
 *
 * @Param td
 * @Param arg
 *
 * @Returns
 */
static Status cmd_ot_check_thread_fun(const thread_t *thread)
{
    Status   ret  = S_OK;
    cmd_req_t *req = NULL;
    int time_wait = 0;

    clist_iter iter;
    clist_iter iter_ot;

    UNUSED(thread);

    while (!thread_is_quit()) {
        sleep_ms(CMD_OT_CHECK_PERIOD);
        time_wait += CMD_OT_CHECK_PERIOD;
        if(time_wait < CMD_OT_CHECK_TIME) {
            continue;
        } else {
            time_wait = 0;
        }

        cmd_list_req_lock();

        iter = clist_begin(g_cmd_list_req);
        while(!clist_iter_is_end(&iter))  {

            iter_ot = iter;
            clist_iter_to_next(&iter);

            req = (cmd_req_t*)clist_iter_obj(&iter_ot);
            if(cmd_get_req_type(&(req->cmd)) == CMD_REQ_TYPE_SYNC) {
                continue;
            }

            if(req->is_sent
            && req->time_left <= 0){
                log_dbg("cmd idx:%d(code:%d) timeout, left:%d timeout:%d!",
                            cmd_get_idx(&(req->cmd)),
                            cmd_get_code(&(req->cmd)),
                            req->time_left,
                            req->time_ot);

                /*
                 * 该命令已经等待超时了，
                 * 把该命令从等待列表中删除，
                 * 然后移动到超时列表中
                 */
                clist_pop(&iter_ot);

                cmd_list_ot_lock();

                clist_append(g_cmd_list_ot, req);

                cmd_list_ot_unlock();
                csem_up(g_sem_ot_cnt);
            } else {
                if(req->is_sent){
                    /* 只有已经发送的才进行超时处理，减去轮询周期 */
                    req->time_left -= CMD_OT_CHECK_PERIOD;
                }
            }
        }

        cmd_list_req_unlock();
    }

    return ret;
}

/**
 * @Brief  超时命令处理
 *
 * @Param cmd 需要处理的超时命令
 */
static  void cmd_process_timeout(cmd_t *cmd)
{
    cmd_routine_t *routine  = NULL;
    cmd_code_t     cmd_code = 0;

    cmd_code = cmd_get_code(cmd);

    log_dbg("timeout cmd, dev:%s code:%d, cmd_id:%d",
            cmd_get_dev_addr_name(cmd), cmd_code, cmd_get_idx(cmd));

    routine = find_cmd_routine_resp(cmd_code);
    if(NULL != routine && NULL != routine->callback_ot){
        routine->callback_ot(cmd);
    } else {
        log_err("No such cmd(%d) timeout routine!", cmd_code);
    }
}

/**
 * @Brief  超时命令处理线程入口函数
 *         等待超时命令列表中有超时命令，然后处理该命令
 *
 * @Param td
 * @Param arg
 *
 * @Returns
 */
static Status cmd_ot_proc_thread_fun(const thread_t *thread)
{
    cmd_req_t  *req  = NULL;

    UNUSED(thread);

    while(!thread_is_quit()){
        if(csem_down_timed(g_sem_ot_cnt, TIME_100MS) != 0) {
            continue;
        }

        cmd_list_ot_lock();
        req = (cmd_req_t*)clist_pop_front(g_cmd_list_ot);
        cmd_list_ot_unlock();

        if(NULL != req){
            cmd_process_timeout(&(req->cmd));

            cobj_free(req);
        }
    }

    return S_OK;
}

void cmd_set_data(cmd_t *cmd, const void *data, uint32_t len)
{
    cmd_set_body_type(cmd, CMD_BODY_TYPE_DATA);
    cmd_set_data_len(cmd, len);
    cmd->body.data = (uint8_t*)malloc(len);
    memcpy(cmd->body.data, data, len);
}

uint8_t* cmd_get_data(cmd_t *cmd)
{
    return CMD_GET_BODY_DATA(cmd);
}

void cmd_req_set_data(cmd_req_t *cmd_req, const void *data, uint32_t len)
{
    cmd_set_data(&(cmd_req->cmd), data, len);
}

void cmd_param_add_tlv_value(cmd_t *cmd, uint8_t tag, tlv_value_t *value)
{
    if(NULL == CMD_GET_BODY_TLVP(cmd)) {
        cmd->body.tlvp = tlvp_new();
    }

    tlvp_add(CMD_GET_BODY_TLVP(cmd), tag, value);
}

#define cmd_param_add_value_macro(cmd, tag, data_type, data_value) \
    tlv_value_t tlv_value;  \
    memset(&tlv_value, 0, sizeof(tlv_value_t)); \
    tlv_value.is_deep     = false;  \
    tlv_value.is_ptr      = false;  \
    tlv_value.is_takeover = false;  \
    tlv_value.length      = sizeof(data_type);  \
    tlv_value.value.data_##data_type = data_value;  \
    cmd_param_add_tlv_value(cmd, tag, &tlv_value)
#define cmd_param_add_data_macro(cmd, tag, __is_deep, __is_takeover, \
                                 data_type, data_ptr, data_len) \
    tlv_value_t tlv_value;  \
    tlv_value.is_deep     = __is_deep;  \
    tlv_value.is_ptr      = true;  \
    tlv_value.is_takeover = __is_takeover;  \
    tlv_value.length      = data_len;  \
    tlv_value.value.data_##data_type = data_ptr;  \
    cmd_param_add_tlv_value(cmd, tag, &tlv_value)

cmd_param_add_declare(u8,  uint8_t)
{
    cmd_param_add_value_macro(cmd, tag, uint8_t, value);
}

cmd_param_add_declare(u16, uint16_t)
{
    uint16_t value_htons = htons(value);
    cmd_param_add_value_macro(cmd, tag, uint16_t, value_htons);
}

void  cmd_add_resp_status(cmd_t *cmd, uint16_t status)
{
    cmd_param_add_u16(cmd, TAG_RESP_STATUS, status);
}

cmd_param_add_declare(u32, uint32_t)
{
    uint32_t value_htonl = htonl(value);
    cmd_param_add_value_macro(cmd, tag, uint32_t, value_htonl);
}

cmd_param_add_declare(u64, uint64_t)
{
    /* uint32_t value_htonl = htonl(value); */
    cmd_param_add_value_macro(cmd, tag, uint64_t, value);
}

cmd_param_add_declare(s8,  int8_t)
{
    cmd_param_add_value_macro(cmd, tag, int8_t, value);
}

cmd_param_add_declare(s16, int16_t)
{
    int16_t value_htons = htons(value);
    cmd_param_add_value_macro(cmd, tag, int16_t, value_htons);
}

cmd_param_add_declare(s32, int32_t)
{
    int32_t value_htonl = htonl(value);
    cmd_param_add_value_macro(cmd, tag, int32_t, value_htonl);
}

cmd_param_add_declare(s64, int64_t)
{
    /* int32_t value_htonl = htonl(value); */
    cmd_param_add_value_macro(cmd, tag, int64_t, value);
}

cmd_param_add_declare(bool, bool)
{
    cmd_param_add_value_macro(cmd, tag, bool, value);
}

cmd_param_add_declare(float, float)
{
    cmd_param_add_value_macro(cmd, tag, float, value);
}

cmd_param_add_declare(time, time_t)
{
    int32_t value_htonl = htonl(value);
    cmd_param_add_value_macro(cmd, tag, int32_t, value_htonl);
}

cmd_param_add_declare(str, char*)
{
    cmd_param_add_data_macro(cmd, tag, false, false, str, value, strlen(value) + 1);
}

void cmd_param_add_str_deep(cmd_t *cmd, uint8_t tag, const char *str)
{
    cmd_param_add_data_macro(cmd, tag, true, false, str_const, str, strlen(str) + 1);
}

void cmd_param_add_str_takeover(cmd_t *cmd, uint8_t tag, char *str)
{
    cmd_param_add_data_macro(cmd, tag, false, true, str, str, strlen(str) + 1);
}

void cmd_param_add_data(cmd_t *cmd, uint8_t tag, const void* value, uint32_t len)
{
    cmd_param_add_data_macro(cmd, tag, false, false, data_const, value, len);
}

void cmd_param_add_data_deep(cmd_t *cmd, uint8_t tag,
                             const void* value, uint32_t len)
{
    cmd_param_add_data_macro(cmd, tag, true, false, data_const, value, len);
}

void cmd_param_add_data_takeover(cmd_t *cmd, uint8_t tag,
                                 void* value, uint32_t len)
{
    cmd_param_add_data_macro(cmd, tag, false, true, data, value, len);
}

bool cmd_param_contain(const cmd_t *cmd, uint8_t tag)
{
    return tlvp_contain(CMD_GET_BODY_TLVP(cmd), tag);
}

#define cmd_param_get_value_macro(cmd, tag, data_type, default_value)   \
    tlv_value_t value;  \
    data_type   value_ret;  \
    if(!cmd_param_contain(cmd, tag)) return default_value;  \
    tlvp_get_value(CMD_GET_BODY_TLVP(cmd), tag, &value); \
    value_ret = value.value.data_##data_type

cmd_param_get_declare(u8,  uint8_t)
{
    cmd_param_get_value_macro(cmd, tag, uint8_t, default_value);
    return value_ret;
}

cmd_param_get_declare(u16, uint16_t)
{
    cmd_param_get_value_macro(cmd, tag, uint16_t, default_value);
    return ntohs(value_ret);
}

uint16_t cmd_get_resp_status(const cmd_t *cmd)
{
    return cmd_param_get_u16(cmd, TAG_RESP_STATUS, S_OK);
}

cmd_param_get_declare(u32, uint32_t)
{
    cmd_param_get_value_macro(cmd, tag, uint32_t, default_value);
    return value_ret;
}

cmd_param_get_declare(u64, uint64_t)
{
    cmd_param_get_value_macro(cmd, tag, uint64_t, default_value);
    return value_ret;
}

cmd_param_get_declare(s8,  int8_t)
{
    cmd_param_get_value_macro(cmd, tag, int8_t, default_value);
    return value_ret;
}

cmd_param_get_declare(s16, int16_t)
{
    cmd_param_get_value_macro(cmd, tag, int16_t, default_value);
    return value_ret;
}

cmd_param_get_declare(s32, int32_t)
{
    cmd_param_get_value_macro(cmd, tag, int32_t, default_value);
    return value_ret;
}

cmd_param_get_declare(s64, int64_t)
{
    cmd_param_get_value_macro(cmd, tag, int64_t, default_value);
    return value_ret;
}

cmd_param_get_declare(bool, bool)
{
    cmd_param_get_value_macro(cmd, tag, bool, default_value);
    return value_ret;
}

cmd_param_get_declare(float, float)
{
    cmd_param_get_value_macro(cmd, tag, float, default_value);
    return value_ret;
}

cmd_param_get_declare(time, time_t)
{
    cmd_param_get_value_macro(cmd, tag, int32_t, default_value);
    return value_ret;
}

const char* cmd_param_get_str(const cmd_t *cmd, uint8_t tag, const char *default_str)
{
    tlv_value_t value;

    if(!cmd_param_contain(cmd, tag)) return default_str;

    tlvp_get_value(CMD_GET_BODY_TLVP(cmd), tag, &value);
    return value.value.data_str;
}

char*  cmd_param_copy_str(const cmd_t *cmd, uint8_t tag, char *str, uint32_t len)
{
    uint32_t len_copy = 0;

    len_copy = cmd_param_copy_data(cmd, tag, str, len);
    str[len_copy - 1] = '\0';   /* 设置最后一个字符为结束字符 */
    return str;
}

uint32_t  cmd_param_get_data(const cmd_t *cmd, uint8_t tag, uint8_t **data)
{
    tlv_value_t value;

    if(!cmd_param_contain(cmd, tag)) return 0;

    tlvp_get_value(CMD_GET_BODY_TLVP(cmd), tag, &value);
    *data   = (uint8_t*)value.value.data_data;

    return value.length;
}

uint32_t cmd_param_copy_data(const cmd_t *cmd, uint8_t tag,
                             void *data, uint32_t length)
{
    tlv_value_t value;
    uint32_t len_copy = length;

    if(!cmd_param_contain(cmd, tag)) return 0;

    tlvp_get_value(CMD_GET_BODY_TLVP(cmd), tag, &value);
    if(len_copy > value.length) len_copy = value.length;

    memcpy(data, value.value.data_data, len_copy);

    return len_copy;
}

#if 0
/*
 * 远程CLI命令，这里采用了全局变量，因此一个程序中只允许运行一个实例
 */
static bool   g_remote_cli_finish = false;
static cstr  *g_remote_cli_str    = NULL;
static Status g_remote_cli_status = S_OK;
static Status cmd_process_cli_resp(cmd_t *cmd_resp)
{
    const char *resp_str = NULL;

    resp_str = cmd_param_get_str(cmd_resp, TAG_CLI_RESP_CONTENT, NULL);
    if(NULL != g_remote_cli_str){
        cstr_append(g_remote_cli_str, "%s", resp_str);
    }

    g_remote_cli_status = cmd_get_error(cmd_resp);
    g_remote_cli_finish = true;
    return S_OK;
}

static void cli_remote(const cli_arg_t *arg, cstr *cli_str)
{
    const char *ni = NULL;
    char remote_cmd[512] = {0};
    dev_addr_t *dev_addr = NULL;
    cmd_t   *cmd_req = NULL;
    cmd_request_opt_t opt;
    uint32_t i = 0;
    Status  ret = S_OK;
    uint8_t  wait_cnt = 30;
    uint32_t argc = cli_arg_cnt(arg);

    if(argc < 2){
        cstr_append(cli_str, "Param Error!, Usage: sh ni cli_cmd argv...\n");
        return;
    }

    /*  获取要操作的仪器 */
    ni = cli_arg_get_str(arg, 0, "");

    dev_addr = dev_addr_mgr_get(ni);
    if(NULL == dev_addr){
        cstr_append(cli_str, "No such device:%s\n", ni);
        return;
    }

    /*  组装远端新的命令 */
    for (i = 0; i < argc - 1; i++) {
        sprintf(remote_cmd, "%s%s ",
                remote_cmd, cli_arg_get_str(arg, i + 1, ""));
    }

    opt.dev_addr = dev_addr;
    opt.cmd_code = CMD_CODE_CLI;
    opt.cmd_idx  = dev_addr_get_next_cmd_idx(dev_addr);
    opt.ot_ms    = TIME_1S * 15;

    cmd_req = cmd_new_req(&opt);
    cmd_param_add_str_deep(cmd_req, TAG_CLI_REQ_CMD, remote_cmd);

    g_remote_cli_finish = false;
    g_remote_cli_str    = cli_str;
    g_remote_cli_status = S_OK;

    ret = cmd_send_request(cmd_req);
    if(ret != S_OK){
        cstr_append(cli_str, "Send Cmd to remote dev failed, return code:%d\n", ret);
        return;
    }

    /*  等待应答 */
    while(!g_remote_cli_finish){
        --wait_cnt;
        if(wait_cnt > 0){
            sleep_ms(500);
        } else {
            break;
        }
    }

    g_remote_cli_str = NULL;
    if(!g_remote_cli_finish){
        cstr_append(cli_str, "Remote cmd exe timeout!\n");
        /* CmdReqDelAllByCode(CMD_CODE_CLI); */
        return ;
    }

    if(g_remote_cli_status != S_OK){
        cstr_append(cli_str, "Remote cmd exe error, return code:%d!\n",
                             g_remote_cli_status);
        return;
    }
}
#endif

static void cmd_proc_echo(const cmd_t *cmd_req, cmd_t *cmd_resp)
{
    uint32_t data_len = cmd_get_data_len(cmd_req);
    uint32_t i = 0;
    uint8_t *data = NULL;
    uint8_t *data_resp = NULL;

    data = CMD_GET_BODY_DATA(cmd_req);
    data_resp = (uint8_t*)malloc(data_len);
    for(i = 0; i < data_len; ++i) {
        data_resp[i] = (~data[i]);
    }

    cmd_set_data(cmd_resp, data_resp, data_len);
    free(data_resp);
}

static void cmd_proc_txtest(const cmd_t *cmd_req, cmd_t *cmd_resp)
{

}

void cmd_init(void)
{
    uint8_t i = 0;

    ex_memzero(&g_cmd_head_ops, sizeof(g_cmd_head_ops));

    g_cmd_head_ops[CMD_TYPE_COMMON].cb_get_size = cmd_head_common_get_size;

    for(i = 0; i < CMD_PRIOR_CNT; ++i) {
        g_cmd_list_recv[i] = clist_new();
    }
    g_routine_list_req  = chash_new();
    g_routine_list_resp = chash_new();
    g_cmd_list_req      = clist_new();
    g_cmd_list_ot       = clist_new();

    /* 初始化信号量 */
    g_sem_ot_cnt   = csem_new(0);
    g_sem_recv_cnt = csem_new(0);
    g_sem_recv     = cmutex_new();

#if 0
    /*
     * 注册命令处理的CLI命令
     */
    cli_regist("sh",   cli_remote,
              "run remote cli command.", "sh ni cmd<CR>");

    /* 注册远程CLI 命令 */
    cmd_routine_regist_resp(CMD_CODE_CLI, cmd_process_cli_resp, NULL);
#endif
    cmd_routine_regist_req(CMD_CODE_PING, NULL);
    cmd_routine_regist_req(CMD_CODE_TX_TEST, cmd_proc_txtest);
    cmd_routine_regist_req(CMD_CODE_ECHO, cmd_proc_echo);

    thread_new("cmd_recv",     cmd_recv_proc_thread_fun, NULL);
    thread_new("cmd_ot_check", cmd_ot_check_thread_fun, NULL);
    thread_new("cmd_ot_proc",  cmd_ot_proc_thread_fun, NULL);
}

void cmd_release(void)
{
    uint8_t i = 0;

    sleep_ms(100);

    chash_free(g_routine_list_req);
    chash_free(g_routine_list_resp);
    clist_free(g_cmd_list_req);
    for(i = 0; i < CMD_PRIOR_CNT; ++i) {
        clist_free(g_cmd_list_recv[i]);
    }
    clist_free(g_cmd_list_ot);

    csem_free(g_sem_ot_cnt);
    csem_free(g_sem_recv_cnt);
    cmutex_free(g_sem_recv);
}
