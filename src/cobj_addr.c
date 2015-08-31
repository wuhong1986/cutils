/* {{{
 * =============================================================================
 *      Filename    :   addr.c
 *      Description :
 *      Created     :   2015-04-23 14:04:54
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include "cobj_addr.h"
#include "command.h"
#include "ex_memory.h"
#include "cobj.h"

struct addr_s {
    time_t  time_create;    /* 创建的时间 */
    time_t  time_keepalive; /* 上次保活的时间 */

    addr_type_t *addr_type;
    void *addr_info;    /* 地址信息，具体类型的地址自定义 */

    /* ========================================================================
     *        send
     * ===================================================================== */
    cmutex *mutex_send;

    /* =======================================================================
     *        recv
     * ===================================================================== */
    cmutex *mutex_recv;
    cmd_recv_buf_t recv_buf;

    dev_addr_t *dev_addr;   /* 该地址所属于的仪器地址 */
};

static void addr_free(void *obj)
{
    addr_t *addr = (addr_t*)obj;

    cobj_free(addr->addr_info);
    cmutex_free(addr->mutex_send);
    cmutex_free(addr->mutex_recv);
}

static cobj_ops_t cobj_ops_addr = {
    .name = "Addr",
    .obj_size = sizeof(addr_t),
    .cb_destructor = addr_free,
};

addr_t* addr_new(struct dev_addr_s *dev_addr,
                 struct addr_type_s *addr_type,
                 void *addr_info)
{
    addr_t *addr = ex_malloc_one(addr_t);

    cobj_set_ops(addr, &cobj_ops_addr);

    addr->dev_addr   = dev_addr;
    addr->addr_type  = addr_type;
    addr->addr_info  = addr_info;
    addr->mutex_send = cmutex_new();
    addr->mutex_recv = cmutex_new();

    return addr;
}

struct addr_type_s *addr_get_addr_type(addr_t *addr)
{
    return addr->addr_type;
}

void *addr_get_addr_info(addr_t *addr)
{
    return addr->addr_info;
}

uint8_t addr_get_prior(addr_t *addr)
{
    return addr->addr_type->prior;
}

struct cmd_recv_buf_s *addr_get_recv_buf(addr_t *addr)
{
    return &(addr->recv_buf);
}

void addr_lock_recv(addr_t *addr)
{
    cmutex_lock(addr->mutex_recv);
}

void addr_unlock_recv(addr_t *addr)
{
    cmutex_unlock(addr->mutex_recv);
}

void addr_lock_send(addr_t *addr)
{
    cmutex_lock(addr->mutex_send);
}

void addr_unlock_send(addr_t *addr)
{
    cmutex_unlock(addr->mutex_send);
}

bool addr_is_same_type(addr_t *addr, uint8_t type)
{
    addr_type_t *addr_type = addr_get_addr_type(addr);
    return addr_type->type == type;
}

bool addr_is_available(addr_t *addr)
{
    addr_type_t *addr_type = addr_get_addr_type(addr);

    return addr_type->ops.cb_available(addr_get_addr_info(addr));
}

bool addr_is_tx_abnormal(addr_t *addr)
{
    addr_type_t *addr_type = addr_get_addr_type(addr);

    return addr_type->ops.cb_abnormal(addr_get_addr_info(addr));
}

int  addr_send(addr_t *addr, const void *data, uint32_t len)
{
    int ret = 0;
    addr_type_t *addr_type = addr_get_addr_type(addr);

    addr_lock_send(addr);
    ret = addr_type->ops.cb_send(addr_get_addr_info(addr), data, len);
    addr_unlock_send(addr);

    return ret;
}

void addr_recv(addr_t *addr, const void *data, uint32_t len)
{
    /* 调用命令接收数据接口 */
    cmd_receive(addr, data, len);
}

void addr_keepalive(addr_t *addr)
{
    addr->time_keepalive = time(NULL);
}

struct dev_addr_s* addr_get_dev_addr(addr_t *addr)
{
    return addr->dev_addr;
}

const char *addr_get_dev_name(const addr_t *addr)
{
    return dev_addr_get_name(addr->dev_addr);
}

const char *addr_get_type_name(const addr_t *addr)
{
    addr_type_t *addr_type = addr_get_addr_type((addr_t*)addr);
    if(NULL == addr_type->name) return "Undefined";
    else return addr_type->name;
}
