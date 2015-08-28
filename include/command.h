#ifndef CMD_H_201409100909
#define CMD_H_201409100909
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   cmd.h
 *      Description :
 *      Created     :   2014-09-10 09:09:33
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

#include "cdefine.h"
#include "command_typedef.h"
#include "dev_addr_mgr.h"
#include "tlv.h"
#include "cobj_cmd_routine.h"

/* 所有命令都包含的公共TAG */
#define TAG_DEST_SLOT   (0x7F)  /* 目的槽号， 当包含此TAG时
                                   命令处理程序需判断是否是本槽，
                                   如果不是则需要转发 */
#define TAG_FORWARD_OVERTIME    (0x7E)  /* 转发超时时间单位ms */
#define TAG_RESP_STATUS     (0x7D)
/*
 * 默认命令处理函数
 * 当在命令处理列表中找不到相应的处理函数时调用下面的处理函数进行处理
 */
#define CMD_CODE_DEFAULT        (0)     /* 0 命令为默认命令码 */
#define CMD_CODE_CLI            (1)     /* CLI 命令 */
#define CMD_CODE_PING           (0x7F00)
#define CMD_CODE_DEBUG          (0x7F01)
#define TAG_CLI_REQ_CMD         (0)
#define TAG_CLI_RESP_CONTENT    (0)

#define CMD_REQ_TIMEOUT_DEFAULT    TIME_10S
#define CMD_REQ_TIMEOUT_NO_RESP    0

#define CMD_HEAD2_MAX_LEN   32

typedef enum cmd_prior_e
{
    CMD_PRIOR_HIGH = 0,
    CMD_PRIOR_MID,
    CMD_PRIOR_LOW,
    CMD_PRIOR_CNT
} cmd_prior_t;

typedef enum cmd_type_e
{
    CMD_TYPE_COMMON = 0,
    CMD_TYPE_WITH_TRANSFER,
    CMD_TYPE_MAX = 15
} cmd_type_t;

typedef enum cmd_error_e
{
    CMD_E_OK          = 0,
    CMD_E_NO_ROUTINE  = 1,
    CMD_E_TIMEOUT     = 13,
    CMD_E_SEND_FAILED = 4
}cmd_error_t;

typedef enum cmd_router_e
{
    CMD_ROUTER_TYPE_NO_PATH = 0,
    CMD_ROUTER_TYPE_PATH,
} cmd_router_t;

typedef enum cmd_body_type_e
{
    CMD_BODY_TYPE_TLV = 0,
    CMD_BODY_TYPE_DATA,
    CMD_BODY_TYPE_MAX = 7
} cmd_body_type_t;

typedef enum cmd_req_type_e
{
    CMD_REQ_TYPE_ASYNC = 0,
    CMD_REQ_TYPE_SYNC
} cmd_req_type_t;

#ifdef WIN32
#pragma pack(1)
#endif

typedef struct cmd_head_1_s
{
    cmd_type_t      cmd_type:5;
    cmd_req_type_t  req_type:1;
    cmd_prior_t     prior:2;
    cmd_error_t     error:5;
    cmd_body_type_t body_type:3;
    uint16_t        cmd_code;
    uint16_t        cmd_idx;
    uint16_t        data_len;
} cmd_head_1_t;

typedef struct cmd_head_s{
    cmd_head_1_t part_1;
    uint8_t      part_2[CMD_HEAD2_MAX_LEN];    /* 最多32个字节 */
    uint8_t      check_sum;     /* 首部的校验码 */
} ATTRIBUTE_PACKED cmd_head_t;

#ifdef WIN32
#pragma pack()
#endif

/**
 * @Brief  命令接收状态
 */
typedef enum cmd_recv_status_e {
    CMD_RECV_S_START_BYTE = 0,
    CMD_RECV_S_HEAD_1,
    CMD_RECV_S_HEAD_2,
    CMD_RECV_S_HEAD_CHECKSUM,
    CMD_RECV_S_BODY_START,
    CMD_RECV_S_BODY,
    CMD_RECV_S_FINISHED
}cmd_recv_status_t;

/**
 * @Brief  命令接收缓存，对于像TCP/IP 这种面向流的数据
 *         有时一个数据包并未包含一整个命令
 */
typedef struct cmd_recv_buf_s {
    cmd_recv_status_t recv_status;      /* 当前命令的接收状态 */
    cmd_head_t        recv_head;
    // cmd_head_1_t      recv_head_1;
    // uint8_t           recv_head_2[CMD_HEAD2_MAX_LEN];
    uint8_t           recv_head_len_1;    /* 命令头部已接收长度 */
    uint8_t           recv_head_len_2;    /* 命令头部已接收长度 */
    uint32_t          recv_data_len;    /* 数据长度 */
    uint32_t          recv_data_offset; /* 当前数据接收的偏移距 */
    uint8_t          *recv_data;        /* 接收的数据 */
}cmd_recv_buf_t;

typedef struct cmd_s cmd_t;
typedef void   (*cmd_callback_resp_post)(const cmd_t *cmd_req, cmd_t *cmd_resp);
typedef void   (*cmd_callback_set_dev_addr)(cmd_t *cmd_resp);

typedef void     (*cmd_head_cb_get_void)(const void *head);
typedef bool     (*cmd_head_cb_get_bool)(const void *head);
typedef uint32_t (*cmd_head_cb_get_common)(const void *head);
typedef void     (*cmd_head_cb_set_bool)(void *head, bool val);
typedef void     (*cmd_head_cb_set_common)(void *head, uint32_t val);
typedef void     (*cmd_head_cb_copy)(const void *head, uint8_t *data, uint32_t len);

typedef struct cmd_head_ops_s
{
    cmd_head_cb_copy        cb_copy;

    cmd_head_cb_get_common  cb_get_mac_src;
    cmd_head_cb_get_common  cb_get_mac_desc;
    cmd_head_cb_get_common  cb_get_size;

    cmd_head_cb_set_common  cb_set_mac_src;
    cmd_head_cb_set_common  cb_set_mac_desc;
} cmd_head_ops_t;

typedef union cmd_body_u
{
    tlvp_t  *tlvp;
    uint8_t *data;  /* 此类型数据不需要释放，它直接指向接收缓存，由其它地方释放 */
} cmd_body_t;

struct cmd_s {
    dev_addr_t *dev_addr;

    cmd_head_t  head;
    cmd_body_t  body;

    cmd_callback_resp_post cb_resp_post;
};

/**
 * @Brief  请求命令节点、列表结构体
 */
typedef struct cmd_req_s
{
    COBJ_HEAD_VARS;

    cmd_t   cmd;

    bool    is_sent;
    time_t  time_req;   /* 请求时间 */
    time_t  time_sent;  /* 成功发送时间 */

    uint32_t time_ot;    /* 超时总时间 */
    int32_t  time_left;

    int     retry_cnt;
    int     retry_left;
} cmd_req_t;

void cmd_head_set_ops(uint8_t head_type, const cmd_head_ops_t *ops);

void cmd_routine_regist_resp(uint32_t cmd_code,
                             cmd_callback_resp cb_resp,
                             cmd_callback_ot   cb_ot);
void cmd_routine_regist_req(uint32_t cmd_code, cmd_callback_req cb_req);

cmd_req_t *cmd_new_req(dev_addr_t *dev_addr, cmd_code_t code);
void cmd_req_free(cmd_req_t *cmd_req);

cmd_t *cmd_new_resp(cmd_t *cmd_req);
void cmd_set_resp_post(cmd_t *cmd_resp, cmd_callback_resp_post cb);
void cmd_set_dev_addr(cmd_callback_set_dev_addr cb);

void cmd_free(cmd_t *cmd);
cmd_code_t cmd_get_code(const cmd_t *cmd);
cmd_idx_t cmd_get_idx(const cmd_t *cmd);
void cmd_set_idx(cmd_t *cmd, cmd_idx_t idx);
void     cmd_set_error(cmd_t *cmd, cmd_error_t error);
cmd_error_t cmd_get_error(const cmd_t *cmd);
cmd_prior_t cmd_get_prior(const cmd_t *cmd);
void cmd_set_prior(cmd_t *cmd, cmd_prior_t prior);
void cmd_set_req_async(cmd_t *cmd);
void cmd_set_req_sync(cmd_t *cmd);

Status cmd_send_request(cmd_req_t *cmd_req);
Status cmd_send_resp(cmd_t *cmd);
void   cmd_receive(addr_t *addr, const void *data, uint32_t len);

bool cmd_param_contain(const cmd_t *cmd, uint8_t tag);

void  cmd_add_resp_status(cmd_t *cmd, uint16_t status);
#define cmd_param_add_declare(name, data_type)   \
        void cmd_param_add_##name(cmd_t *cmd, uint8_t tag, data_type value)
cmd_param_add_declare(u8,  uint8_t);
cmd_param_add_declare(u16, uint16_t);
cmd_param_add_declare(u32, uint32_t);
cmd_param_add_declare(u64, uint64_t);
cmd_param_add_declare(s8,  int8_t);
cmd_param_add_declare(s16, int16_t);
cmd_param_add_declare(s32, int32_t);
cmd_param_add_declare(s64, int64_t);
cmd_param_add_declare(bool, bool);
cmd_param_add_declare(float, float);
cmd_param_add_declare(time, time_t);
cmd_param_add_declare(str, char*);
void cmd_param_add_str_deep(cmd_t *cmd, uint8_t tag, const char *str);
void cmd_param_add_str_takeover(cmd_t *cmd, uint8_t tag, char *str);
void cmd_param_add_data(cmd_t *cmd, uint8_t tag, const void *value, uint32_t len);
void cmd_param_add_data_deep(cmd_t *cmd, uint8_t tag,
                             const void* value, uint32_t len);
void cmd_param_add_data_takeover(cmd_t *cmd, uint8_t tag,
                                 void* value, uint32_t len);

uint16_t cmd_get_resp_status(const cmd_t *cmd);
#define cmd_param_get_declare(name, data_type)  \
        data_type cmd_param_get_##name(const cmd_t *cmd, uint8_t tag, \
                                       data_type default_value)
cmd_param_get_declare(u8,  uint8_t);
cmd_param_get_declare(u16, uint16_t);
cmd_param_get_declare(u32, uint32_t);
cmd_param_get_declare(u64, uint64_t);
cmd_param_get_declare(s8,  int8_t);
cmd_param_get_declare(s16, int16_t);
cmd_param_get_declare(s32, int32_t);
cmd_param_get_declare(s64, int64_t);
cmd_param_get_declare(bool, bool);
cmd_param_get_declare(float, float);
cmd_param_get_declare(time, time_t);
const char* cmd_param_get_str(const cmd_t *cmd, uint8_t tag, const char *default_str);
char* cmd_param_copy_str(const cmd_t *cmd, uint8_t tag, char *str, uint32_t len);
uint32_t cmd_param_get_data(const cmd_t *cmd, uint8_t tag, uint8_t **data);
uint32_t cmd_param_copy_data(const cmd_t *cmd, uint8_t tag,
                             void *data, uint32_t length);
void cmd_param_finish(cmd_t *cmd);
uint32_t cmd_sizeof(const cmd_t *cmd);

void cmd_init(void);
void cmd_release(void);
#ifdef __cplusplus
}
#endif
#endif  /* CMD_H_201409100909 */
