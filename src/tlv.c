/* {{{
 * =============================================================================
 *      Filename    :   ctlv.c
 *      Description :
 *      Created     :   2014-09-06 20:09:14
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clist.h"
#include "clog.h"
#include "tlv.h"
#include "clog.h"
#include "ex_memory.h"

struct tlvp_s
{
    clist   *list;
};

static bool tlv_dbg_on = false;

#define OBJ_TO_TLV(obj) ((tlv_t*)(obj))

static void cobj_tlv_free(void *obj)
{
    tlv_t *tlv = (tlv_t*)obj;

    if(tlv->value.is_deep || tlv->value.is_takeover) {
        free(tlv->value.value.data_data);
    }
}

static cobj_ops_t cobj_tlv_ops = {
    .name = "Tlv",
    .obj_size = sizeof(tlv_t),
    .cb_destructor = cobj_tlv_free,
};

tlv_t* tlv_new(uint8_t tag, const tlv_value_t *value)
{
    tlv_t *tlv = ex_malloc_one(tlv_t);

    cobj_set_ops(tlv, &cobj_tlv_ops);

    tlv->tag.is_ptr = value->is_ptr;
    tlv->tag.tag    = tag & (0x7F);
    tlv->length     = value->length;
    tlv->value      = *value;
    if(value->is_deep) {
        tlv->value.value.data_data = malloc(value->length);
        memcpy(tlv->value.value.data_data, value->value.data_data, value->length);
    }

    return tlv;
}

static uint32_t  get_length_from_data(const uint8_t *data, uint32_t *length_size)
{
    uint32_t   len_size = 0;
    uint32_t   length   = 0;
    uint32_t   i        = 0;

    if(data[0] & (1 << 7)){ /* 第一个Byte的BIT7为1，获取表示长度的比特  */
        len_size = data[0] & (0x7F);
        for (i = 0; i < len_size; i++) {
            length = length * BYTE_MAX_VALUE + data[len_size - i];
        }
        ++len_size; /*  加上表示字节长度的字节 */
    } else {
        length   = data[0];
        len_size = 1;
    }

    if(NULL != length_size) *length_size = len_size;
    /*LoggerDebug("get length:%d", length);*/

    return length;
}

tlv_t* tlv_from_data(const void *tlv_data, uint32_t *size)
{
    uint32_t len_size = 0;
    uint32_t tlv_size = 0;
    const uint8_t *data = (const uint8_t*)tlv_data;
    tlv_t *tlv = ex_malloc_one(tlv_t);

    cobj_set_ops(tlv, &cobj_tlv_ops);

    /* Tag */
    tlv->tag.is_ptr = ((data[0] & ( 1 << 7)) >> 7);
    tlv->tag.tag    = (data[0]  & 0x7F);
    tlv_size += 1;

    /* Length */
    tlv->length = get_length_from_data(&(data[1]), &len_size);
    tlv_size += len_size;

    /* Value */
    if(!tlv->tag.is_ptr){
        memcpy(&(tlv->value.value), &(data[tlv_size]), tlv->length);
    } else {
        tlv->value.length = tlv->length;
        tlv->value.value.data_data_const = &(data[tlv_size]);
    }
    tlv_size += tlv->length;

    if(NULL != size) *size = tlv_size;

    return tlv;
}

static inline uint8_t tlv_get_tag(const tlv_t *tlv)
{
    return OBJ_TO_TLV(tlv)->tag.tag;
}

/**
 * @Brief  计算length域在发送时占的空间大小
 *
 * @Param length
 *
 * @Returns
 */
uint32_t  tlv_get_length_sizeof(const tlv_t *tlv)
{
    uint32_t   length    = 1;
    uint32_t   len_left  = 0;

    if(OBJ_TO_TLV(tlv)->length <= TLV_LENGTH_1BYTE_MAX_VALUE)   return 1;

    len_left  = OBJ_TO_TLV(tlv)->length;
    while(len_left > 0){
        len_left /= 256;
        ++length;
    }

    return length;
}

/**
 * @Brief  计算value域在发送时占的空间大小
 *
 * @Param length
 *
 * @Returns
 */
uint32_t  tlv_get_value_sizeof(const tlv_t *tlv)
{
    return OBJ_TO_TLV(tlv)->length;
}

uint32_t tlv_get_sizeof(const tlv_t *tlv)
{
    return 1    /* TAG 始终占1个字节 */
         + tlv_get_length_sizeof(tlv)
         + tlv_get_value_sizeof(tlv);
}

static const tlv_t* tlvp_get_tlv(const tlvp_t *tlvp, uint8_t tag)
{
    const clist_node *node = NULL;
    const tlv_t*      tlv  = NULL;

    if(NULL == tlvp) return NULL;

    clist_foreach_val(tlvp->list, node, tlv){
        if(tlv_get_tag(tlv) == tag) return tlv;
    }

    return NULL;
}

tlvp_t *tlvp_new(void)
{
    tlvp_t *tlvp = (tlvp_t*)malloc(sizeof(tlvp_t));

    memset(tlvp, 0, sizeof(tlvp_t));

    tlvp->list = clist_new();

    return tlvp;
}

void tlvp_free(tlvp_t *tlvp)
{
    clist_free(tlvp->list);
    free(tlvp);
}

uint32_t tlvp_cnt(const tlvp_t *tlvp)
{
    return clist_size(tlvp->list);
}

bool tlvp_contain(const tlvp_t *tlvp, uint8_t tag)
{
    return tlvp_get_tlv(tlvp, tag) != NULL;
}

void tlvp_get_value(const tlvp_t *tlvp, uint8_t tag, tlv_value_t *value)
{
    const tlv_t *tlv = NULL;

    tlv = tlvp_get_tlv(tlvp, tag);
    if(NULL != tlv){
        *value = tlv->value;
    }
}

int  tlvp_add(tlvp_t *tlvp, uint8_t tag, tlv_value_t *value)
{
    tlv_t *tlv = tlv_new(tag, value);

    if(NULL != tlvp_get_tlv(tlvp, tlv_get_tag(tlv))) {
        log_err("tlv tag(%d) already exist!", tlv_get_tag(tlv));
        cobj_free(tlv);
        return -1;
    }

    clist_append(tlvp->list, tlv);

    return 0;
}

/**
 * @Brief  计算tlvp_t在通过网络发送时所需要传送的数据大小
 *
 * @Param tlvp
 *
 * @Returns   对应的数据大小
 */
uint32_t tlvp_sizeof(const tlvp_t *tlvp)
{
    const clist_node *node = NULL;
    const tlv_t *tlv = NULL;
    uint32_t length  = 0;

    clist_foreach_val(tlvp->list, node, tlv) {
        length += tlv_get_sizeof(tlv);
    }

    return length;
}

static int tlv_to_data(const tlv_t *tlv, void *tlv_data, uint32_t data_len)
{
    uint32_t tag_size    = 0;
    uint32_t length_size = 0;
    uint32_t value_size  = 0;
    uint32_t offset      = 0;
    uint32_t length_left = 0;
    uint8_t  *data       = (uint8_t*)tlv_data;

    tag_size    = 1;
    length_size = tlv_get_length_sizeof(tlv);
    value_size  = tlv_get_value_sizeof(tlv);

    if(tag_size + length_size + value_size > data_len){
        log_err("data len err, %d + %d + %d != %d",
                tag_size, length_size, value_size, data_len);
        return -1;
    }

    /* TAG */
    data[0] |= (tlv->tag.is_ptr << 7);
    data[0] |= (tlv->tag.tag & 0x7F);
    offset = tag_size;

    /* LENGTH */
    if(tlv->length <= TLV_LENGTH_1BYTE_MAX_VALUE){
        data[offset] = tlv->length;
        ++offset;
    } else {
        /*
         * 一个字节不够表示长度时：
         *  第一个字节的最高位 BIT7 = 1，其余BIT0-BIT6表示后面有几个字节表示长度
         */
        data[offset] = (1 << 7) | (length_size - 1);
        ++offset;
        length_left = tlv->length;
        while(length_left > 0){
            if(length_left / BYTE_MAX_VALUE != 0){
                data[offset] = length_left % BYTE_MAX_VALUE;
                length_left  = length_left / BYTE_MAX_VALUE;
            } else {
                data[offset] = length_left;
                length_left = 0;
            }

            ++offset;
            /*printf("length_left:%d\n", length_left);*/
        } /*  end of while */
    }

    if(value_size > 0){
        if(!tlv->tag.is_ptr){
            /* log_dbg("tlv valuexxxx:%08X", tlv->value.value.data_uint32_t); */
            memcpy(&(data[offset]), &(tlv->value.value), value_size);
        } else { /*  数据是指针，拷贝指针指向的内容 */
            memcpy(&(data[offset]), tlv->value.value.data_data, value_size);
        }
        offset += value_size;
    }

    return 0;
}

void tlvp_to_data(const tlvp_t *tlvp, void *data, uint32_t data_len)
{
    const clist_node *node = NULL;
    const tlv_t      *tlv  = NULL;
    uint32_t tlv_size = 0;
    uint32_t offset   = 0;
    uint32_t i        = 0;

    clist_foreach_val(tlvp->list, node, tlv){
        tlv_size = tlv_get_sizeof(tlv);

        tlv_to_data(tlv, data + offset, tlv_size);

        offset += tlv_size;
    }

    if(tlv_dbg_on) {
        printf("Tlv Package Tx length:%d datas:", data_len);
        for (i = 0; i < data_len; i++) {
            printf("%02X ", ((uint8_t*)data)[i]);
        }
        printf("\n");
    }
}

void tlvp_from_data(tlvp_t *tlvp, const void *data, uint32_t data_len)
{
    tlv_t    *tlv     = NULL;
    uint32_t offset   = 0;
    uint32_t tlv_size = 0;
    uint32_t i        = 0;

    if(tlv_dbg_on) {
        printf("Tlv Package Rx length:%d datas:", data_len);
        for (i = 0; i < data_len; i++) {
            printf("%02X ", ((uint8_t*)data)[i]);
        }
        printf("\n");
        log_dbg("fuck");
    }


    while(offset < data_len){
        tlv = tlv_from_data(data + offset, &tlv_size);

        clist_append(tlvp->list, tlv);

        offset += tlv_size;
    }
}
