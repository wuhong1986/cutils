/* {{{
 * =============================================================================
 *      Filename    :   dev_addr_sock.c
 *      Description :
 *      Created     :   2014-09-13 14:09:43
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/event.h"
#include "event2/thread.h"
#include <errno.h>
#include "clog.h"
#include "ex_memory.h"
#include "ex_socket.h"
#include "ex_assert.h"
#include "dev_addr_sock.h"
#include "dev_addr_mgr.h"
#include "cobj_addr.h"

static struct event_base *g_event_base = NULL;

#define OBJ_TO_ADDR_SOCK(obj) ((addr_sock_t*)(obj))

int  cobj_addr_sock_cmp(const void *obj1, const void *obj2)
{
    const addr_sock_t *addr1 = (const addr_sock_t*)obj1;
    const addr_sock_t *addr2 = (const addr_sock_t*)obj2;
    const struct sockaddr_in *sin_addr1 = &(addr1->sockaddr);
    const struct sockaddr_in *sin_addr2 = &(addr2->sockaddr);

    if(sin_addr1->sin_family != sin_addr2->sin_family){
        return sin_addr1->sin_family - sin_addr2->sin_family;
    }

    if(sin_addr1->sin_addr.s_addr != sin_addr2->sin_addr.s_addr){
        /* printf("addr1:%d addr2:%d", */
        /*        sin_addr1->sin_addr.s_addr, */
        /*        sin_addr2->sin_addr.s_addr); */
        return sin_addr1->sin_addr.s_addr - sin_addr2->sin_addr.s_addr;
    }
    /* printf("addr same hshs\n"); */

    if(addr1->is_fix_port != addr2->is_fix_port )
    {
        return addr1->is_fix_port - addr2->is_fix_port;
    }

    if(addr1->is_fix_port && (sin_addr1->sin_port != sin_addr2->sin_port)) {
        return sin_addr1->sin_port - sin_addr2->sin_port;
    }

    return 0;
}

void cobj_addr_sock_destory(void *obj)
{
    addr_sock_t *addr_sock = (addr_sock_t*)obj;

    if(addr_sock->bev){
        bufferevent_free(addr_sock->bev);
    }
}

cobj_ops_t cobj_ops_addr_sock = {
    .name = "AddrSock",
    .obj_size = sizeof(addr_sock_t),
    .cb_cmp = cobj_addr_sock_cmp,
    .cb_destructor = cobj_addr_sock_destory,
};

addr_sock_t* cobj_addr_sock_new(void)
{
    addr_sock_t *addr_sock = ex_malloc_one(addr_sock_t);

    cobj_set_ops(addr_sock, &cobj_ops_addr_sock);
    addr_sock->fd = SOCK_FD_INVALID;
    addr_sock->bev = NULL;

    return addr_sock;
}

void addr_sock_set_event_base(struct event_base *event_base)
{
    g_event_base = event_base;
}

void addr_sock_copy(void *addr_dest, const void *addr_src)
{
    const addr_sock_t *addr_sock_src = (const addr_sock_t*)addr_src;
    addr_sock_t *addr_sock_dest = (addr_sock_t*)addr_dest;

    *addr_sock_dest = *addr_sock_src;
}

void* addr_sock_new(void *info)
{
    addr_sock_t *addr_sock = NULL;

    addr_sock = ex_malloc_one(addr_sock_t);
    addr_sock->fd = SOCK_FD_INVALID;
    addr_sock_copy(addr_sock, info);

    return addr_sock;
}

int  addr_sock_send(void *obj_addr_info, const void *data, uint32_t len)
{
    addr_sock_t *addr_sock = (addr_sock_t*)(obj_addr_info);
    struct bufferevent *bev = addr_sock->bev;

    uint32_t max_write = 0;
    int sent_total = 0;
    uint32_t sent = 0;
    uint32_t left = len;

    bufferevent_lock(bev);

    while(left > 0) {
        max_write = bufferevent_get_max_to_write(bev);
        sent = left;
        if(sent > max_write) sent = max_write;

        if(sent > 0){
            /* LoggerDebug("@@-- sent %d/%d bytes, send:%d max:%d.", */
            /*             sent_total, length, sent, max_write); */
            if(0 == bufferevent_write(bev, data + sent_total, sent)){
                /* send OK */
                sent_total += sent;
                left -= sent;
                /* LoggerDebug("@@!! sent %d/%d bytes, send:%d max:%d.", */
                /*             sent_total, length, sent, max_write); */
            } else {
                log_info("bufferevent_write  failed: %s", strerror(errno));
                addr_sock->fd = SOCK_FD_INVALID;
                break;
            }
        } else {
            log_dbg("wait...");
        }
    }

    /* bufferevent_flush(bev, EV_WRITE, BEV_FLUSH); */

    bufferevent_unlock(bev);

    return sent_total;
}

bool addr_sock_available(void *obj_addr_info)
{
    addr_sock_t *addr_sock = (addr_sock_t*)(obj_addr_info);

    return (SockFd)(addr_sock->fd) != SOCK_FD_INVALID;
}

bool addr_sock_abnormal(void *obj_addr_info)
{
    addr_sock_t *addr_sock = (addr_sock_t*)(obj_addr_info);

    return addr_sock->is_tx_abnormal;
}

static void cb_conn_read(struct bufferevent *bev, void *user_data)
{
    void*    buffer  = NULL;
    uint32_t buf_len = 0;
    addr_t   *addr = (addr_t*)user_data;

    struct evbuffer *buf_in = bufferevent_get_input(bev);

    /* read data frome buffer in */
    buf_len = evbuffer_get_length(buf_in);
    buffer = calloc(1, buf_len);
    bufferevent_read(bev, buffer, buf_len);

    /* put data to addr recv buffer, and translate to command format */
    addr_recv(addr, buffer, buf_len);

    free(buffer);
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    addr_t   *addr = (addr_t*)user_data;
    addr_sock_t* addr_sock = OBJ_TO_ADDR_SOCK(addr_get_addr_info(addr));
    bool  is_error = false;

    if (events & BEV_EVENT_EOF) {
        is_error = true;
        log_dbg("Connection(dev:%s type:%s) closed.", addr_get_dev_name(addr),
                                                      addr_get_type_name(addr));
    } else if (events & BEV_EVENT_ERROR) {
        is_error = true;
        log_dbg("Got an error on the connection: %s",
                strerror(errno));/*XXX win32*/
    } else {
        log_dbg("conn_eventcb other");
    }

    if(is_error){
        addr_sock->fd  = SOCK_FD_INVALID;
        addr_sock->bev = NULL;
        bufferevent_free(bev);
        dev_addr_mgr_proc_offline(addr);
    }

    /* None of the other events can happen here, since we haven't enabled
     * timeouts */
    /* bufferevent_free(bev); */
}

void addr_sock_connect(addr_t *addr, void *obj_addr_info)
{
    addr_sock_t *addr_sock = OBJ_TO_ADDR_SOCK(obj_addr_info);

    struct bufferevent *bev = NULL;
    struct sockaddr_in *saddr = NULL;

    addr_sock->fd   = Socket(AF_INET, SOCK_STREAM, 0);
    saddr           = &(addr_sock->sockaddr);
    if(connect(addr_sock->fd,
               (struct sockaddr *)saddr,
               sizeof(struct sockaddr_in)) < 0){
        log_err("connect to remote failed!Addr:%s:%d, reason:%s\n",
                    inet_ntoa(saddr->sin_addr),
                    ntohs(saddr->sin_port),
                    strerror(errno));
        SOCKET_CLOSE(addr_sock->fd);
        return ;
    }

    log_dbg("connect to remote Addr:%s:%d, fd = %d Addr:0x%0X",
            inet_ntoa(saddr->sin_addr), ntohs(saddr->sin_port),
            addr_sock->fd, saddr->sin_addr);

#ifdef QJ_BOARD_ANDROID
    bev = bufferevent_socket_new(g_event_base,
                                 addr_sock->fd,
                                 BEV_OPT_CLOSE_ON_FREE);
#else
#if 0
    bev = bufferevent_socket_new(g_event_base,
                                 addr_sock->fd,
                                 BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
#else
    bev = bufferevent_socket_new(g_event_base,
                                 addr_sock->fd,
                                 BEV_OPT_CLOSE_ON_FREE);
#endif
#endif
    if(bev == NULL){
        log_err("bufferevent socket new failed!");
        ex_assert(0);
        return ;
    }
    bufferevent_setcb(bev, cb_conn_read, NULL, conn_eventcb, addr);
    bufferevent_enable(bev, EV_WRITE | EV_READ);

    addr_sock->bev = bev;
}

void addr_sock_init(void)
{
    addr_ops_t ops;

    ops.cb_available = addr_sock_available;
    ops.cb_connect   = addr_sock_connect;
    ops.cb_send      = addr_sock_send;
    ops.cb_abnormal  = addr_sock_abnormal;

    addr_type_set_name(ADDR_TYPE_ETHERNET, "ETHERNET");
    addr_type_set_prior(ADDR_TYPE_ETHERNET,  ADDR_PRIOR_ETHERNET);
    addr_type_set_ops(ADDR_TYPE_ETHERNET, &ops);
}
