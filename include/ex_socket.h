#ifndef EX_SOCKET_H_201410021910
#define EX_SOCKET_H_201410021910
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   ex_socket.h
 *      Description :
 *      Created     :   2014-10-02 19:10:04
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */

#include <stdint.h>
#ifdef  __linux__
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
typedef     int32_t     SockFd;
#define     SOCK_FD_INVALID     -1
#define		SOCKET_ERROR		-1
#else   /*  windows */
#include <winsock2.h>
#include <windows.h>
typedef     SOCKET  SockFd;
#define     SOCK_FD_INVALID     INVALID_SOCKET
#define     socklen_t           int
#endif

/* ==========================================================================
 *                             Socket 相关
 * ======================================================================= */
SockFd Socket(int domain, int type, int protocol);
uint32_t  get_ip_addr(const char *dev_name);

#ifdef WIN32
#define 	SOCKET_CLOSE(sock_fd) 	\
    do{\
        if(SOCK_FD_INVALID != (SockFd)sock_fd){\
            closesocket(sock_fd);\
            sock_fd = SOCK_FD_INVALID;\
        }\
    }while(0)
#else
#define 	SOCKET_CLOSE(sock_fd) 	\
    do{\
        if(SOCK_FD_INVALID != sock_fd){\
            close(sock_fd);\
            sock_fd = SOCK_FD_INVALID;\
        }\
    }while(0)
#endif

#ifdef __cplusplus
}
#endif
#endif  /* EX_SOCKET_H_201410021910 */
