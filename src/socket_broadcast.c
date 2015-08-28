/* {{{
 * =============================================================================
 *      Filename    :   socket_broadcast.c
 *      Description :
 *      Created     :   2014-10-06 15:10:36
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

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

#include "cthread.h"
#include "clog.h"
#include "ex_memory.h"
#include "ex_assert.h"
#include "ex_time.h"
#include "ex_string.h"
#include "ex_socket.h"
#include "dev_addr_mgr.h"
#include "dev_addr_sock.h"
#include "socket_broadcast.h"

typedef struct broad_info_s {
    const char *name;
    uint32_t period;
    uint16_t port_bc;
    uint16_t port_resp;
    uint16_t port_connect;

    socket_resp_msg_t msg;
}broad_info_t;
/*  */
/* static void broad_info_free(void *ptr) */
/* { */
/*     broad_info_t *info = ptr; */
/*     ex_free(info->msg_body); */
/*     free(info); */
/* } */

static void sock_broadcast(uint32_t addr_ip, const broad_info_t *bc_info)
{
    struct sockaddr_in sock_addr;
    SockFd sock_fd = SOCK_FD_INVALID;

    int32_t ret = 0;
    int opt = 1;

    sock_fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(SOCK_FD_INVALID == sock_fd){
        log_err("create bc send socket failed:%s", strerror(errno));
        return;
    }

    ret = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST,
                     (const char *)&opt, sizeof(int32_t));
    if(ret == SOCKET_ERROR){
        log_err("setsockopt failed:%s", strerror(errno));
        return ;
    }

    opt = 1;
    ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&opt, sizeof(int32_t));
    if(ret == SOCKET_ERROR){
        log_err("setsockopt failed:%s", strerror(errno));
        return ;
    }

    sock_addr.sin_family      = AF_INET;
    sock_addr.sin_port        = htons(bc_info->port_bc);
    sock_addr.sin_addr.s_addr = addr_ip;
    memset(&(sock_addr.sin_zero), 0, 8);

    /* log_dbg("BC to:%s.", inet_ntoa(sock_addr.sin_addr)); */

    sendto(sock_fd, NULL, 0, 0, (struct sockaddr *)&(sock_addr), sizeof(sock_addr));

    SOCKET_CLOSE(sock_fd);
}

static Status sock_bc_rx_th_cb(const thread_t *thread)
{
    broad_info_t *info = NULL;
    socklen_t addrlen = 0;
    struct  sockaddr_in saddr_local;
    struct  sockaddr_in saddr_remote;
    struct  sockaddr_in saddr_resp;
    int     iRetRecv  = 0;
    int32_t ret       = 0;
    char    msg[32] = {0};
    SockFd  sock_resp = SOCK_FD_INVALID;
    SockFd  sock_recv = SOCK_FD_INVALID;

    info = (broad_info_t*)thread_get_arg(thread);

    memset(&saddr_local, 0, sizeof(struct sockaddr_in));
    sock_recv = Socket(AF_INET, SOCK_DGRAM, 0);

    memset(&saddr_local, 0, sizeof(struct sockaddr_in));
    saddr_local.sin_family      = AF_INET;
    saddr_local.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr_local.sin_port        = htons(info->port_bc);

    if(bind(sock_recv, (struct sockaddr *)&saddr_local, sizeof(struct sockaddr_in)) < 0){
        log_dbg("Bind port:%d failed:%s", info->port_bc, strerror(errno));
        SOCKET_CLOSE(sock_recv);
        return ERR_SOCK_BIND_FAILED;
    }

    sock_resp = Socket(AF_INET, SOCK_DGRAM, 0);

    while(!thread_is_quit()){
        if(!readable_timed(sock_recv, TIME_100MS)) { continue; }

        /* log_dbg("wait for recv"); */
        addrlen = sizeof(struct sockaddr_in);   /* 一定要加上这句，
                                                   否则接收到的地址为0.0.0.0 */
        iRetRecv = recvfrom(sock_recv, msg, sizeof(msg), 0,
                            (struct sockaddr *)&(saddr_remote), &addrlen);
        if(iRetRecv < 0){
            log_err("recvfrom failed:%s", strerror(errno));
            ex_assert(0);
            continue;
        }

        /*  返回应答信息给服务端 */
        memset(&saddr_resp, 0, sizeof(struct sockaddr_in));
        saddr_resp.sin_family = AF_INET;
        saddr_resp.sin_port   = htons(info->port_resp);
        saddr_resp.sin_addr   = saddr_remote.sin_addr;

        ret = sendto(sock_resp,
                     (const char*)(&(info->msg)), sizeof(socket_resp_msg_t), 0,
                     (struct sockaddr *)&(saddr_resp), sizeof(saddr_resp));
        if(ret != sizeof(socket_resp_msg_t)) {
            log_err("sendto failed:%s", strerror(errno));
        } else {
            /* log_dbg("send to center"); */
        }
    }

    SOCKET_CLOSE(sock_resp);
    SOCKET_CLOSE(sock_recv);

    free(info);

    return S_OK;
}

static Status sock_bc_tx_th_cb(const thread_t *thread)
{
    broad_info_t *info = NULL;
#ifdef __linux__
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa_netmask = NULL;
    struct sockaddr_in *sa_ip      = NULL;
#else /* windows */
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = 0;
#endif
    uint32_t  ip_netmask = 0;
    uint32_t  ip_addr = 0;
    uint32_t  ip_bc = 0;
    int time_wait = 0;

    info = (broad_info_t*)thread_get_arg(thread);

    while(!thread_is_quit()){
        sleep_ms(TIME_100MS);
        if(time_wait >= info->period) {
            time_wait = 0;
        } else {
            time_wait += TIME_100MS;
            continue;
        }

#ifdef __linux__    /* Linux */
        getifaddrs (&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family == AF_INET) {
            /* && strcmp(ifa->ifa_name, "lo") != 0 #<{(| 过滤掉本地回环地址 |)}>#) { */
                sa_netmask = (struct sockaddr_in *) ifa->ifa_netmask;
                sa_ip      = (struct sockaddr_in *) ifa->ifa_addr;
                ip_netmask = sa_netmask->sin_addr.s_addr;
                ip_addr    = sa_ip->sin_addr.s_addr;
                ip_bc      = (ip_netmask & ip_addr) | (~ip_netmask);

                sock_broadcast(ip_bc, info);
            }
        }

        freeifaddrs(ifap);
#else   /* windows */

        /* 先获取大小 */
        GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);

        if((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
            pAdapter = pAdapterInfo;
            while (pAdapter) {
                if(strncmp(pAdapter->Description, "Bluetooth", strlen("Bluetooth")) == 0){
                    pAdapter = pAdapter->Next;
                    continue;
                }

                ip_addr    = inet_addr(pAdapter->IpAddressList.IpAddress.String);
                ip_netmask = inet_addr(pAdapter->IpAddressList.IpMask.String);
                ip_bc      = (ip_netmask & ip_addr) | (~ip_netmask);;
                sock_broadcast(ip_bc, info);

                pAdapter = pAdapter->Next;
            }
        }

        ex_free(pAdapterInfo);
        ulOutBufLen = 0;
#endif
    }

    free(info);

    return S_OK;
}

static Status sock_bc_resp_th_cb(const thread_t *thread)
{
#define     MAX_MSG_LEN_BC_RECV     256
    socklen_t addrlen = 0;
    int     iRetRecv;
    socket_resp_msg_t msg;
    int     opt      = 1;
    struct sockaddr_in sockaddrLocal;
    broad_info_t info;
    SockFd sock_bc_resp = SOCK_FD_INVALID;
    dev_addr_t  *dev_addr = NULL;
    addr_sock_t  *addr_sock = NULL;

    info = *((broad_info_t*)thread_get_arg(thread));

    memset(&sockaddrLocal, 0, sizeof(struct sockaddr_in));
    sock_bc_resp = Socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_bc_resp == SOCK_FD_INVALID){
        return ERR_SOCK_OPEN_FAILED;
    }

    memset(&sockaddrLocal, 0, sizeof(struct sockaddr_in));
    sockaddrLocal.sin_family      = AF_INET;
    sockaddrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddrLocal.sin_port        = htons(info.port_resp);

    if(SOCKET_ERROR == setsockopt(sock_bc_resp, SOL_SOCKET, SO_REUSEADDR,
                                  (const char *)&opt, sizeof(opt))){
        log_err("set sockopt failed:%s", strerror(errno));
        return ERR_SOCK_SET_OPT_FAILED;
    }

    if(bind(sock_bc_resp, (struct sockaddr *)&sockaddrLocal,
            sizeof(struct sockaddr_in)) < 0){
        log_dbg("Bind port:%d failed:%s", info.port_resp, strerror(errno));
        SOCKET_CLOSE(sock_bc_resp);
        return ERR_SOCK_BIND_FAILED;
    }

    addr_sock = cobj_addr_sock_new();

    while(!thread_is_quit()){
        if(!readable_timed(sock_bc_resp, TIME_100MS)) { continue; }

        addr_sock->fd  = SOCK_FD_INVALID;
        addr_sock->bev = NULL;

        addrlen = sizeof(struct sockaddr_in);   /* 一定要加上这句，
                                                   否则接收到的地址为0.0.0.0 */
        iRetRecv = recvfrom(sock_bc_resp, (char*)&msg, sizeof(socket_resp_msg_t), 0,
                            (struct sockaddr *)&(addr_sock->sockaddr), &addrlen);
        if(iRetRecv < 0){
            log_err("recvfrom failed:%s", strerror(errno));
            break;
        }

        if(dev_addr_mgr_is_support_dev_type(msg.dev_type)) {
            if(strcmp(msg.dev_name, "") == 0) {
                strcpy(msg.dev_name, "Undefined");
            }

            addr_sock->sockaddr.sin_port = htons(info.port_connect);
            addr_sock->is_fix_port = true;

            dev_addr = dev_addr_mgr_add(msg.dev_name, msg.dev_type);
            dev_addr_add(dev_addr, ADDR_TYPE_ETHERNET, addr_sock);
        } else {
            /* log_warn("recv unsupport dev type: %d name: %s", */
            /*          msg.dev_type, msg.dev_name); */
        }

    }

    cobj_free(addr_sock);

    return S_OK;
}

Status socket_bc_tx_start(const char *name, uint16_t port_bc,
                          uint16_t port_resp, uint16_t port_connect)
{
    Status ret = S_OK;
    char   th_name[64] = {0};
    broad_info_t *info = ex_malloc_one(broad_info_t);

    info->name         = name;
    info->period       = TIME_1S * 2;
    info->port_bc      = port_bc;
    info->port_resp    = port_resp;
    info->port_connect = port_connect;

    sprintf(th_name, "sock_bc_resp(%s)", name);
    ret = thread_new(th_name, sock_bc_resp_th_cb, info);

    sprintf(th_name, "sock_bc_tx(%s)", name);
    ret = thread_new(th_name, sock_bc_tx_th_cb, info);

    /* log_dbg("Socket port: %d  %d %d", port_bc, port_resp, port_connect); */

    return ret;
}

Status socket_bc_rx_start(const char *name,
                          uint16_t port_bc, uint16_t port_resp,
                          const socket_resp_msg_t *msg)
{
    Status ret = S_OK;
    char   th_name[64] = {0};
    broad_info_t *info = ex_malloc_one(broad_info_t);

    info->name      = name;
    info->port_bc   = port_bc;
    info->port_resp = port_resp;
    info->msg       = *msg;

    sprintf(th_name, "sock_bc_rx(%s)", name);
    ret = thread_new(th_name, sock_bc_rx_th_cb, info);

    return ret;
}
