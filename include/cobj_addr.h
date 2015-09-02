#ifndef addr_H_201504231404
#define addr_H_201504231404
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   addr.h
 *      Description :
 *      Created     :   2015-04-23 14:04:59
 *      Author      :   Wu Hong
 * =============================================================================
 }}} */

#include <stdint.h>
#include "cobj.h"

struct dev_addr_s;
struct addr_type_s;
typedef struct addr_s addr_t;

addr_t* addr_new(struct dev_addr_s *dev_addr,
                 struct addr_type_s *addr_type,
                 void *addr_info);
struct addr_type_s *addr_get_addr_type(addr_t *addr);
struct dev_addr_s* addr_get_dev_addr(addr_t *addr);
void *addr_get_addr_info(addr_t *addr);
uint8_t addr_get_prior(addr_t *addr);
struct cmd_recv_buf_s *addr_get_recv_buf(addr_t *addr);
bool addr_is_available(addr_t *addr);
bool addr_is_tx_abnormal(addr_t *addr);
bool addr_is_same_type(addr_t *addr, uint8_t type);
void addr_lock_send(addr_t *addr);
void addr_unlock_send(addr_t *addr);
void addr_lock_recv(addr_t *addr);
void addr_unlock_recv(addr_t *addr);
int  addr_send(addr_t *addr, const void *data, uint32_t len);
void addr_recv(addr_t *addr, const void *data, uint32_t len);
void addr_keepalive(addr_t *addr);
const char *addr_get_dev_name(const addr_t *addr);
const char *addr_get_type_name(const addr_t *addr);


#ifdef __cplusplus
}
#endif
#endif  /* addr_H_201504231404 */

