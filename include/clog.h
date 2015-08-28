#ifndef QJ_LOG_H_201310091010
#define QJ_LOG_H_201310091010
#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 *      Filename    :   clog.h
 *      Description :
 *      Created     :   2013-10-09 10:10:00
 *      Author      :    Wu Hong
 * =============================================================================
 */
#include <stdbool.h>
#include <time.h>

#define LOG_DIR_DEFAULT  (NULL)

/**
 * @Brief  日志级别
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_OFF,
    LOG_LEVEL_CNT = LOG_LEVEL_OFF,
    LOG_LEVEL_ALL
}log_level_t;

/**
 * @Brief  日志输出内容
 */
typedef enum log_item_e{
    LOG_ITEM_LEVEL = 0,   /*  输出级别信息 */
    LOG_ITEM_DATE,        /*  输出日期 */
    LOG_ITEM_GPS,         /*  輸出GPS時間 */
    LOG_ITEM_FILE,        /*  输出文件名, 行号 */
    LOG_ITEM_FUNC,        /*  输出函数名 */
    LOG_ITEM_PIT,         /*  输出线程信息 */
    LOG_ITEM_BODY,        /*  输出消息体 */
    LOG_ITEM_ENDL,        /*  输出尾部 */
    LOG_ITEM_CNT,         /*  个数 */
}log_item_t;

/*
 * 日志输出的写入函数，该函数告诉日志系统字符串str写到哪里
 */
typedef void (*log_out_cb_write)(const char *str);
/*
 * 日志系统Flush函数，用于把缓存内容强制写入到指定的设备中
 * 有的设备不需要flush 则可以设置为NULL
 */
typedef void (*log_out_cb_flush)(void);

typedef struct log_out_s{
    const char  *name;   /*  该日志输出的名称 */

    log_out_cb_write cb_write;   /* 该日志输出的写入函数 */
    log_out_cb_flush cb_flush;   /*  该日志输出的flush函数 */
}log_out_t;

typedef time_t (*callback_log_gps_time)(void);

void log_set_gps_time_callback(callback_log_gps_time cb);

/**
 * @Brief  添加一个日志输出对象，比如在MFC中调试用的TRACE
 *
 *		定义一个输出函数
 *      void logger_mfc_write(const char *str)
 *      {
 *          TRACE(str);
 *      }
 *      再调用本函数添加即可实现日志系统输出到TRACE
 *      log_add_out("MFC Trace", logger_mfc_write, NULL);
 *
 * @Param name          该日志输出的名称
 * @Param write_fun     该输出的写入函数
 * @Param flush_fun     该输出的flush函数
 *
 * @Returns     该输出在日志系统中的引用
 */
log_out_t* log_add_out(const char *name,
                       log_out_cb_write cb_write,
                       log_out_cb_flush cb_flush);

/**
 * @Brief  设置保存日志文件的目录
 *
 * @Param dir   需要保存的目录, 为NULL则采用默认目录
 */
void  log_set_dir(const char *dir);
/**
 * @Brief  获取日志文件保存的目录
 *
 * @Returns
 */
const char *log_get_dir(void);

/**
 * @Brief  日志系统的统一接口，该接口给LoggerXXX调用
 *
 * @Param file  文件名
 * @Param line  行号
 * @Param func  函数名
 * @Param level 日志级别
 * @Param fmt   格式化字符串
 * @Param ...   参数
 *
 */
void log_printf(const char *file, int line,
                const char *func, log_level_t level,
                const char *fmt, ...);

/**
 * @Brief  输出相应的日志级别内容
 *          此处必须用宏，这样才能得到对应的__FILE__ __LINE__ __FUNCTION__
 *
 * @Param fmt 格式化字符串
 * @Param ...    参数
 *
 */
#ifdef __GNUC__
/*  GCC编译__VA_ARGS__有告警 */
#define log_dbg(fmt,args...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_DEBUG, fmt,##args)
#define log_info(fmt,args...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_INFO,  fmt,##args)
#define log_warn(fmt,args...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_WARN,  fmt,##args)
#define log_err(fmt,args...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_ERROR, fmt,##args)
#define log_assert(fmt,args...)   do{\
                                        log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_ERROR, fmt,##args);\
                                        ex_assert(0);\
                                    }while(0)
#else
/*  VC编译##args有告警 */
#define log_dbg(fmt, ...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...)	log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define log_assert(fmt,...)   do{\
                                    log_printf(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_ERROR, fmt,##__VA_ARGS__);\
                                    ex_assert(0);\
                                }while(0)
#endif

#define LOG_OUT_ALL  NULL
/*
 * 获取日志标准输出
 */
log_out_t *log_out_std(void);
/*
 * 获取日志文件输出
 */
log_out_t *log_out_file(void);
/**
 * @Brief  设置日志输出级别
 *
 * @Param level 输出的最低级别
 */
void log_set_level(log_level_t level);

/**
 * @Brief  设置相应级别输出时包含的内容
 *
 * @Param item 要设置的项
 *
 * @Param enable  是否使能
 *                true: 使能
 *                false: 不使能
 */
void log_item_enable(log_item_t item, bool enable);

void log_init(void);
/**
 * @Brief  释放所占资源，在退出系统时调用
 */
void log_release(void);

#ifdef __cplusplus
}
#endif
#endif  /* QJ_LOG_H_201310091010 */
