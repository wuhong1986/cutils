#ifndef EX_TIME_H_201410021910
#define EX_TIME_H_201410021910
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   ex_time.h
 *      Description :
 *      Created     :   2014-10-02 19:10:58
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <unistd.h>
#include <time.h>
#ifdef __linux__
#include <sys/time.h>
#endif

#define TIME_1MS        1
#define TIME_10MS       (10 * TIME_1MS)
#define TIME_100MS      (10 * TIME_10MS)
#define TIME_500MS      (5 * TIME_100MS)
#define TIME_1S         (10 * TIME_100MS)
#define TIME_5S         (5 * TIME_1S)
#define TIME_10S        (10 * TIME_1S)
#define TIME_1MIN       (60 * TIME_1S)
#define TIME_1HOUR      (60 * TIME_1MIN)
#define TIME_8HOUR      (8 * TIME_1HOUR)

#define TIME_1S_SEC     (1)
#define TIME_1MIN_SEC   (60 * TIME_1S_SEC)
#define TIME_1HOUR_SEC  (60 * TIME_1MIN_SEC)
#define TIME_8HOUR_SEC  (8 * TIME_1HOUR_SEC)

int sleep_ms(int ms);

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp);
#endif

int  ex_time_now_us(void);
int  ex_time_now_ms(void);
void ex_time_now_set(time_t time_now);
struct tm* ex_time_now_tm(void);
time_t  ex_time_now_utc(void);
time_t  ex_time_t_from_complie_dt(const char *date, const char *time);

#ifdef __cplusplus
}
#endif
#endif  /* EX_TIME_H_201410021910 */
