/*
 * =============================================================================
 *      Filename    :   qj_comm_sock.c
 *      Description :
 *      Created     :   2013-10-10 09:10:15
 *      Author      :    Wu Hong
 * =============================================================================
 */
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>   /* qt 需要添加libiphlpapi 库 */
#else
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/thread.h"

#include "cthread.h"
#include "clog.h"
#include "cdefine.h"
#include "ex_socket.h"
#include "ex_memory.h"
#include "ex_assert.h"
#include "ex_time.h"
#include "socket_libevent.h"
#include "dev_addr_mgr.h"
#include "dev_addr_sock.h"

/* #define SOCKET_LIBEVENT_ENABLE_THREAD */

static struct event_base *g_event_base = NULL;
static struct evconnlistener *g_event_listener_async = NULL;
static struct evconnlistener *g_event_listener_cli   = NULL;
/* static struct event g_listen_ev; */

static void cb_conn_write(struct bufferevent *bev, void *user_data)
{
#if 0
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
        printf("send OK\n");
        /* bufferevent_free(bev); */
    }
#else
    UNUSED(bev);
    UNUSED(user_data);
#endif
}

static void cb_conn_read_async(struct bufferevent *bev, void *user_data)
{
    void*    buffer  = NULL;
    uint32_t buf_len = 0;
    addr_t   *addr   = (addr_t*)user_data;

    struct evbuffer *buf_in = bufferevent_get_input(bev);

    /* read data frome buffer in */
    buf_len = evbuffer_get_length(buf_in);
    buffer = calloc(1, buf_len);
    bufferevent_read(bev, buffer, buf_len);

    /* put data to addr recv buffer, and translate to command format */
    addr_recv(addr, buffer, buf_len);
    free(buffer);
}

static void cb_conn_read_cli(struct bufferevent *bev, void *user_data)
{
    void*    buffer  = NULL;
    uint32_t buf_len = 0;
    struct evbuffer *buf_in = bufferevent_get_input(bev);

    /* read data frome buffer in */
    buf_len = evbuffer_get_length(buf_in);
    buffer = calloc(1, buf_len);
    bufferevent_read(bev, buffer, buf_len);

    log_dbg("recv command: %s", (char*)buffer);

    /* put data to addr recv buffer, and translate to command format */
    free(buffer);
}

static void conn_eventcb_async(struct bufferevent *bev, short events, void *user_data)
{
    addr_t* addr = (addr_t*)user_data;
    addr_sock_t *addr_sock = (addr_sock_t*)addr_get_addr_info(addr);
    bool  is_error = false;

    if (events & BEV_EVENT_EOF) {
        is_error = true;
        log_dbg("Connection(dev:%s type:%s) closed.", addr_get_dev_name(addr),
                                                      addr_get_type_name(addr));
    } else if (events & BEV_EVENT_ERROR) {
        is_error = true;
        log_dbg("Got an error on the connection: %s",
                strerror(errno));/*XXX win32*/
    } else { log_dbg("conn_eventcb other");
    }

    if(is_error){
        addr_sock->fd  = SOCK_FD_INVALID;
        addr_sock->bev = NULL;
        bufferevent_free(bev);
    }

    /* None of the other events can happen here, since we haven't enabled
     * timeouts */
    /* bufferevent_free(bev); */
}

static void conn_eventcb_cli(struct bufferevent *bev, short events, void *user_data)
{
    bool  is_error = false;

    if (events & BEV_EVENT_EOF) {
        is_error = true;
        log_dbg("cli connection closed.");
    } else if (events & BEV_EVENT_ERROR) {
        is_error = true;
        log_dbg("Got an error on the connection: %s",
                strerror(errno));/*XXX win32*/
    } else {
        log_dbg("conn_eventcb other");
    }

    if(is_error){
        bufferevent_free(bev);
    }

    /* None of the other events can happen here, since we haven't enabled
     * timeouts */
    /* bufferevent_free(bev); */
}

static void listener_cli_cb(struct evconnlistener *listener,
                            evutil_socket_t fd, struct sockaddr *sa,
                            int socklen, void *user_data)
{
    struct event_base  *base = (struct event_base*)user_data;
    struct bufferevent *bev = NULL;
    struct sockaddr_in *sa_in = (struct sockaddr_in*)sa;

    UNUSED(listener);
    UNUSED(socklen);

#ifdef SOCKET_LIBEVENT_ENABLE_THREAD
    bev = bufferevent_socket_new(base, fd,
                                 BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
#else
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
#endif
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    log_dbg("accept a cli connect");

    bufferevent_setcb(bev, cb_conn_read_cli, cb_conn_write, conn_eventcb_cli, NULL);
    bufferevent_enable(bev, EV_WRITE | EV_READ);
}

Status socket_cli_request(const char *ip, uint16_t port, const char *request_str)
{
    Status ret = S_OK;
    int fd = 0;
    int write_cnt = 0;
    struct sockaddr_in saddr;

    fd = Socket(AF_INET, SOCK_STREAM, 0);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_aton(ip, (struct in_addr*)&(saddr.sin_addr.s_addr));
    if(connect(fd, (struct sockaddr *)(&saddr), sizeof(struct sockaddr_in)) < 0) {
        log_dbg("connect to %s:%d failed: %s", ip, port, strerror(errno));
    }

    log_dbg("connect to %s:%d OK", ip, port);
    write_cnt = write(fd, request_str, strlen(request_str) + 1);

    SOCKET_CLOSE(fd);

    return ret;
}

static void listener_async_cb(struct evconnlistener *listener,
                              evutil_socket_t fd, struct sockaddr *sa,
                              int socklen, void *user_data)
{
    struct event_base  *base = (struct event_base*)user_data;
    struct bufferevent *bev = NULL;
    char   center_name[64] = {0};
    struct sockaddr_in *sa_in = (struct sockaddr_in*)sa;
    dev_addr_t  *dev_addr  = NULL;
    addr_t  *addr      = NULL;
    addr_sock_t *addr_sock = NULL;
    static int sock_idx = 0;

    UNUSED(listener);
    UNUSED(socklen);

    ex_memzero_one(&addr_sock);

#ifdef SOCKET_LIBEVENT_ENABLE_THREAD
    bev = bufferevent_socket_new(base, fd,
                                 BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
#else
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
#endif
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    sprintf(center_name, "_%d", sock_idx++);
    dev_addr = dev_addr_mgr_add(center_name, DEV_TYPE_UNKNOWN, 0);
    dev_addr_set_addr_mac(dev_addr, ADDR_MAC_NONE);

    addr_sock = cobj_addr_sock_new();
    addr_sock->fd  = fd;
    addr_sock->bev = bev;
    addr_sock->is_fix_port = false;
    addr_sock->sockaddr.sin_addr = sa_in->sin_addr;
    addr_sock->sockaddr.sin_port = sa_in->sin_port;

    addr = dev_addr_add(dev_addr, ADDR_TYPE_ETHERNET, addr_sock);

    log_dbg("Accept connect from:%s, fd = %d.",
            inet_ntoa(addr_sock->sockaddr.sin_addr), fd);

    bufferevent_setcb(bev, cb_conn_read_async, cb_conn_write, conn_eventcb_async, addr);
    bufferevent_enable(bev, EV_WRITE | EV_READ);

    addr_sock->fd  = -1;
    addr_sock->bev = NULL;
    cobj_free(addr_sock);
}

/**
 * @Brief  以太网等待接收请求入口函数
 *
 * @Param th_param
 *
 * @Returns
 */
static struct evconnlistener *socket_listen(uint16_t listen_port,
                                            evconnlistener_cb cb)
{
    struct sockaddr_in saddr_server;
    struct evconnlistener *listener = NULL;

    memset(&saddr_server, 0, sizeof(struct sockaddr_in));
    saddr_server.sin_family      = AF_INET;
    saddr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr_server.sin_port        = htons(listen_port);

    if(!g_event_base) {
        log_err("g_event_base is null");
    }

    listener = evconnlistener_new_bind(g_event_base,
                                       cb,
                                       (void *)g_event_base,
                                       LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
                                       -1,
                                       (struct sockaddr*)&saddr_server,
                                       sizeof(saddr_server));
    if (!listener) {
        log_err("listen port %d failed !\n", listen_port);
        return NULL;
    }

    return listener;
}

void socket_listen_async(uint16_t listen_port)
{
    g_event_listener_async = socket_listen(listen_port, listener_async_cb);
}

void socket_listen_cli(uint16_t listen_port)
{
    g_event_listener_cli = socket_listen(listen_port, listener_cli_cb);
}

/**
 * @Brief  以太网接收线程入口函数
 *
 * @Param th_param
 *
 * @Returns
 */
static Status sock_recv_fun(const thread_t *thread)
{
    UNUSED(thread);

    while(!thread_is_quit()){
        sleep_ms(TIME_100MS * 2);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);
    }

    return S_OK;
}

Status  socket_recv_start(void)
{
    return thread_new("socket receive", sock_recv_fun, NULL);
}

void event_log_print(int severity, const char *msg)
{
    log_dbg("[LIBEVENT:%d]%s",severity, msg);
}

static void socket_quit(void)
{
    event_base_loopbreak(g_event_base);
}

void socket_init(void)
{
    addr_sock_init();
    event_set_log_callback(event_log_print);
#ifdef SOCKET_LIBEVENT_ENABLE_THREAD
#ifdef EVTHREAD_USE_PTHREADS_IMPLEMENTED
#ifndef QJ_BOARD_ANDROID
    evthread_use_pthreads();
#endif
#else
    if(evthread_use_windows_threads()){
        log_err("evthread_use_windows_threads failed!");
    }
#endif
#endif
    g_event_base = event_base_new();
    if(NULL == g_event_base){
        log_err("event base new failed!");
        ex_assert(0);
    }

    addr_sock_set_event_base(g_event_base);
    cli_add_quit_cb(socket_quit);
}

void socket_release(void)
{
    if(g_event_listener_async){
        evconnlistener_free(g_event_listener_async);
    }
    if(g_event_listener_cli){
        evconnlistener_free(g_event_listener_cli);
    }
    event_base_free(g_event_base);
}
