#ifndef EVENT_CONFIG_H_201408230808
#define EVENT_CONFIG_H_201408230808
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   event-config.h
 *      Description :
 *      Created     :   2014-08-23 08:08:05
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#ifdef __linux__
#   ifdef QJ_BOARD_ANDROID
#       include "event-config-android.h"
#   else
#       include "event-config-linux.h"
#   endif
#elif defined WIN32
#   include "event-config-windows.h"
#else
#   error "No such platform libevnt config head file!"
#endif

#ifdef __cplusplus
}
#endif
#endif  /* EVENT-CONFIG_H_201408230808 */
