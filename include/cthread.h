#ifndef QJ_THREAD_H_201310090910
#define QJ_THREAD_H_201310090910
#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 *      Filename    :   cthread.h
 *      Description :
 *      Created     :   2013-10-09 09:10:30
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <time.h>
#include "cobj.h"
#include "ex_errno.h"

/* ==========================================================================
 *        线程名称
 * ======================================================================= */
#define THREAD_NAME_SOCK_BC_SEND    "sock_bc_send"
#define THREAD_NAME_SOCK_BC_RECV    "sock_bc_recv"
#define THREAD_NAME_SOCK_BC_RESP    "sock_bc_resp"
#define THREAD_NAME_LED_CTRL        "led"
#define THREAD_NAME_CMD_RECV        "Cmd_Recv"
#define THREAD_NAME_CMD_OT_CHECK    "Cmd_OT_Check"
#define THREAD_NAME_CMD_OT_PROC     "Cmd_OT_Proc"
#define THREAD_NAME_GPS             "GPS"
#define THREAD_NAME_GPS_DBG         "GPSDbg"
#define THREAD_NAME_ZIG_PACK_MONITOR "Zig Monitor"
#define THREAD_NAME_DSP_UPLOAD      "DSP"

#ifdef __linux__
#include <pthread.h>
typedef pthread_t th_id_t;
#else
#include <windows.h>
typedef HANDLE th_id_t;
#endif

typedef struct thread_s thread_t;

/*  线程回调函数，用来封装不同操作系统 */
typedef Status (*callback_thread)(const thread_t *thread);

/*  线程的信息 */
struct thread_s {
    COBJ_HEAD_VARS;

    char     *name;
    th_id_t   id;
#ifdef WIN32
    DWORD     pid;
#else
    pid_t     pid;
#endif
    int       td;
    void     *arg;

    callback_thread cb;
};

/**
 * @Brief  启动线程
 *
 * @Param thread_name   线程的名称 通过名称来查找线程
 * @param func_init     初始化函数，可以为NULL， 表示不需要初始化
 * @Param func          启动线程后执行的回调函数
 * @Param arg           启动线程所需的参数
 * @Param td            创建后的线程ID，ID < 0为错误值
 *
 * @Returns    0: 创建OK
 *             其他值：   对应的错误码
 */
Status  thread_new(const char *name, callback_thread cb, void *arg);

/**
 * @Brief  关闭线程
 *
 * @Param td
 */
void thread_free(const char *name);
bool thread_is_quit(void);

/**
 * @Brief  线程模块初始化，必须在使用创建函数之前调用
 */
void thread_init(void);
void thread_release(void);

/**
 * @Brief  获取当前线程的名称
 *
 * @Returns
 */
const char *thread_current_name(void);
int thread_current_td(void);
const void *thread_get_arg(const thread_t *thread);
#ifdef __cplusplus
}
#endif
#endif  /* QJ_THREAD_H_201310090910 */
