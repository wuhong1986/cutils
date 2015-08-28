/* {{{
 * =============================================================================
 *      Filename    :   ex_socket.c
 *      Description :
 *      Created     :   2014-10-02 19:10:41
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <errno.h>
#include <string.h>
#ifdef __linux__
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif
#include "ex_socket.h"
#include "clog.h"
#include "cdefine.h"

SockFd Socket(int domain, int type, int protocol)
{
    SockFd fd = socket(domain, type, protocol);
    if(SOCK_FD_INVALID == fd){
#ifdef __linux__
        log_err("socket open failed:%s", strerror(errno));
#else
        log_err("socket open failed:%d", GetLastError());
#endif
    }
    return fd;
}

uint32_t  get_ip_addr(const char *dev_name)
{
#ifdef __linux__
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
#else
    UNUSED(dev_name);
    return 0;
#endif
}
