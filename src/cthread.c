/*
 * =============================================================================
 *      Filename    :   cthread.c
 *      Description :
 *      Created     :   2013-10-09 10:10:55
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <errno.h>
#include <string.h>
#ifdef __linux__
#include <signal.h>
#include <unistd.h>
#ifndef QJ_BOARD_ANDROID
#include <execinfo.h>
#endif
#include <sys/syscall.h>
#else
#include <windows.h>
#endif
#include "cthread.h"
#include "clist.h"
#include "cstring.h"
#include "csem.h"
#include "clog.h"
#include "ccli.h"
#include "cdefine.h"
#include "ex_time.h"
#include "ex_assert.h"

static clist *g_list = NULL;
static int    g_id_last = 0;
static bool   g_is_need_quit = false;

static void cobj_thread_free(void *obj)
{
    thread_t *thread = (thread_t*)obj;

#ifdef WIN32
    TerminateThread(thread->id, 0);
#else
#ifdef QJ_BOARD_ANDROID
    pthread_kill(thread->id, SIGKILL);
#else
    pthread_cancel(thread->id);
#endif
#endif

    free(thread->name);
}

cobj_ops_t cobj_ops_thread = {
    .name = "thread",
    .obj_size = sizeof(thread_t),
    .cb_destructor = cobj_thread_free,
};

inline static void thread_list_lock(void)
{
    clist_lock(g_list);
}

inline static void thread_list_unlock(void)
{
    clist_unlock(g_list);
}

static void  thread_delete(int td)
{
    thread_t    *thread = NULL;
    clist_iter iter = clist_begin(g_list);

    clist_iter_foreach_obj(&iter, thread){
        if(thread->td == td){
            break;
        }
    }

    clist_remove(&iter);
}

/**
 * @Brief  获取当前线程的名称
 *
 * @Returns
 */
const char *thread_current_name(void)
{
#ifdef __linux__
    thread_t   *thread = NULL;
    clist_iter iter = clist_begin(g_list);

    clist_iter_foreach_obj(&iter, thread){
        if(thread->id == pthread_self()){
            return thread->name;
        }
    }
#endif

    return "\"Main\"";
}

int thread_current_td(void)
{
#ifdef __linux__
    thread_t   *thread = NULL;
    clist_iter iter = clist_begin(g_list);

    clist_iter_foreach_obj(&iter, thread){
        if(thread->id == pthread_self()){
            return thread->td;
        }
    }
#endif

    return -1;
}

#ifdef __linux__
static const char *get_name(int td)
{
    thread_t   *thread = NULL;
    clist_iter iter = clist_begin(g_list);

    clist_iter_foreach_obj(&iter, thread){
        if(thread->td == td){
            return thread->name;
        }
    }

    return "\"Maybe Main\"";
}

static cstr *g_bt_str = NULL;
static csem *g_sem_bt = NULL;
#define THREAD_BT_MAX_SIZE  128
extern void get_backtrace(cstr *str);
static void signal_bt_handler(int signum)
{
    int     td = -1;

    csem_lock(g_sem_bt);

    td = thread_current_td();
    cstr_append(g_bt_str, "[Backtrace %d:%s, Signal:%d]\n",
                            td, get_name(td), signum);

    get_backtrace(g_bt_str);

    cstr_append(g_bt_str, "\n");

    csem_unlock(g_sem_bt);
}
#endif

#ifdef WIN32
static DWORD WINAPI thread_cb(void *arg)
#else
static void *thread_cb(void *arg)
#endif
{
    long   ret = S_OK;
    thread_t *thread_info = (thread_t *)arg;

#ifdef __linux__
    thread_info->pid = getpid();

    pthread_detach(pthread_self());

    backtrace_regist(SIGSEGV);
    backtrace_regist(SIGABRT);
    backtrace_regist(SIGPIPE);

    /* 注册打印堆栈信息函数 */
    signal(SIGUSR1, signal_bt_handler);
#endif

    /* LoggerInfo("Start thread id:%d, name:\"%s\".", */
    /*            thread_info->td, */
    /*            thread_info->name); */

    /*  执行定义的回调函数 */
    ret = thread_info->cb(thread_info);

    thread_list_lock();

    thread_delete(thread_info->td);

    thread_list_unlock();

#ifdef WIN32
    return (DWORD)ret;
#else
    return (void *)ret;
#endif
}

Status  thread_new(const char *name, callback_thread cb, void *arg)
{
    thread_t *thread = NULL;

    thread_list_lock();

    thread = (thread_t*)malloc(sizeof(thread_t));

    cobj_set_ops(thread, &cobj_ops_thread);

    thread->td   = g_id_last++;
    thread->name = strdup(name);
    thread->cb   = cb;
    thread->arg  = arg;

    clist_append(g_list, thread);

    /*  根据不同操作系统调用对应的启动线程函数 */
#ifdef WIN32
    thread->id = CreateThread(NULL, 0, thread_cb, (LPVOID)thread,
                              0 , &(thread->pid));
    if(thread->id == 0)

#else
    if(pthread_create(&(thread->id), NULL, thread_cb, thread) != 0)
#endif
    {
        log_assert("Start thread failed:%s", strerror(errno));
        return ERR_TH_CREATE_FAILED;
    }

    thread_list_unlock();

    return S_OK;
}

const void *thread_get_arg(const thread_t *thread)
{
    return thread->arg;
}

void 	thread_free(const char *name)
{
    thread_t   *thread = NULL;
    clist_iter iter;

    thread_list_lock();

    iter = clist_begin(g_list);
    clist_iter_foreach_obj(&iter, thread){
        if(strcmp(thread->name, name) == 0) {
            break;
        }
    }

    if(!clist_iter_is_end(&iter) && thread){
        thread_delete(thread->td);
    }

    thread_list_unlock();
}

static void cli_thread_fun(cli_cmd_t *cmd)
{
    thread_t *thread = NULL;
    clist_iter iter = clist_begin(g_list);

    cli_output(cmd, "%-3s%-24s\n", "ID",  "Name");
    cli_output(cmd, "-----------------------------------------------------\n");

    clist_iter_foreach_obj(&iter, thread){
        cli_output(cmd, "%-3d%-24s\n", thread->td, thread->name);
    }
}

#if 0
static void cli_thread_bt_fun(const cli_arg_t *arg, cstr *cli_str)
{
#ifndef __linux__
    UNUSED(arg);
    cstr_append(cli_str, "Only support linux plateform!\n");
#else
    clist_iter iter = clist_begin(g_list);
    thread_t *thread = NULL;
    int td = -1;
    uint32_t argc = cli_arg_cnt(arg);

    g_bt_str = cstr_new();

    if(argc > 0){
        td = cli_arg_get_int(arg, 0, 0);
    }

    /* send signal to thread */
    clist_iter_foreach_obj(&iter, thread){
        if(td == -1 || td == thread->td){
            pthread_kill(thread->id, SIGUSR1);
        }
    }

    /* sleep 100ms wait for response */
    sleep_ms(500);

    cstr_add(cli_str, g_bt_str);

    cstr_free(g_bt_str);
    g_bt_str = NULL;
#endif
}
#endif

static void thread_quit(void)
{
    printf("Prepare to kill all threads...\n");
    g_is_need_quit = true;
}

bool thread_is_quit(void)
{
#ifdef __linux__
    thread_t   *thread = NULL;
    clist_iter iter;
#endif

    if(g_is_need_quit) {
#ifdef __linux__
        thread_list_lock();

        iter = clist_begin(g_list);
        clist_iter_foreach_obj(&iter, thread){
            if(thread->id == pthread_self()){
                printf("Kill thread: %s ...\n", thread->name);
            }
        }

        thread_list_unlock();
#endif
    }

    return g_is_need_quit;
}

void thread_init(void)
{
    g_list = clist_new();

#ifdef __linux__
    g_sem_bt = cmutex_new();
#endif

    cli_add_quit_cb(thread_quit);

    cli_regist("thread", cli_thread_fun);
    cli_set_brief("thread", "Show system threads info.");
#if 0
    cli_regist("bt",     cli_thread_bt_fun,
              "Show thread backtrace infomation.", "thread_bt [thread id]<CR>");
#endif
}

void thread_release(void)
{
    clist_free(g_list);
#ifdef __linux__
    cmutex_free(g_sem_bt);
#endif
}
