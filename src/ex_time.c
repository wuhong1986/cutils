/* {{{
 * =============================================================================
 *      Filename    :   ex_time.c
 *      Description :
 *      Created     :   2014-10-02 20:10:47
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "ex_time.h"
#include "clog.h"
#include "cdefine.h"

int sleep_ms(int ms)
{
#ifdef __linux__
    return usleep(ms * 1000);
#else
    Sleep(ms);
    return 0;
#endif
}

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    UNUSED(tzp);

    GetLocalTime(&wtm);
    tm.tm_year  = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday  = wtm.wDay;
    tm.tm_hour  = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm.tm_isdst = -1;
    clock       = mktime(&tm);
    tp->tv_sec  = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif

time_t ex_time_now_utc(void)
{
    return time(NULL) - TIME_8HOUR_SEC; /*  减8小时 转化为GPS 时间 */
}

/**
 * @Brief  设置系统时间
 *
 * @Param dt
 *
 * @Returns
 */
void ex_time_now_set(time_t time_now)
{
#ifndef DEBUG_IN_PC
#ifdef __linux__
    struct timeval tv;

    tv.tv_sec  = time_now;
    tv.tv_usec = 0;
    if(settimeofday(&tv, (struct timezone *) 0) < 0){
        log_dbg("Set system datatime error:%s", strerror(errno));
    }

#else
    UNUSED(time_now);
#endif
#else
    UNUSED(time_now);
#endif
}

int ex_time_now_us(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec;
}

int ex_time_now_ms(void)
{
    return ex_time_now_us() / 1000;
}

struct tm* ex_time_now_tm(void)
{
    time_t timeNow = time(NULL);

    return localtime(&timeNow);
}

time_t  ex_time_t_from_complie_dt(const char *date, const char *time)
{
    char s_month[5];
    int month, day, year;
    int hour, min, sec;
    struct tm t;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    memset(&t, 0, sizeof(struct tm));

    sscanf(date, "%s %d %d", s_month, &day, &year);
    sscanf(time, "%d:%d:%d", &hour, &min, &sec);

    month = (strstr(month_names, s_month)-month_names)/3;

    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;
    t.tm_isdst = -1;

    return mktime(&t);
}
