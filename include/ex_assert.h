#ifndef QJ_ASSERT_H_201310090810
#define QJ_ASSERT_H_201310090810
#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 *      Filename    :   qj_assert.h
 *      Description :
 *      Created     :   2013-10-09 08:10:47
 *      Author      :    Wu Hong
 * =============================================================================
 */
void backtrace_regist(int signum);
void ex_assert_do(const char *condition, const char *file, int line);

#define ex_assert(condition)   \
	do{	\
        if(!(condition)){  \
            ex_assert_do(""#condition, __FILE__, __LINE__);	\
        }   \
	}while(0)

void ex_assert_init_cli(void);
#ifdef __cplusplus
}
#endif
#endif  /* QJ_ASSERT_H_201310090810 */
