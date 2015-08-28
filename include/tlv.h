#ifndef CTLV_H_201409061909
#define CTLV_H_201409061909
#ifdef __cplusplus
extern "C" {
#endif

/* {{{
 * =============================================================================
 *      Filename    :   ctlv.h
 *      Description :
 *      Created     :   2014-09-06 19:09:21
 *      Author      :    Wu Hong
 * =============================================================================
 }}} */
#include <stdint.h>
#include <stdbool.h>

#define TLV_LENGTH_1BYTE_MAX_VALUE  (127)
#define BYTE_MAX_VALUE              (256)

typedef enum ctlv_data_type_s {
    CTLV_DATA_TYPE_SIMPLE = 0,  /* 基本数据类型 */
    CTLV_DATA_TYPE_COMPLEX      /* 复杂数据类型，为浅拷贝，只保存地址 */
}tlv_data_type_t;

typedef union  tlv_value_u{
    bool        data_bool;
    bool        data__Bool;
    uint8_t     data_uint8_t;
    int8_t      data_int8_t;
    uint16_t    data_uint16_t;
    int16_t     data_int16_t;
    int32_t     data_int64_t;
    uint32_t    data_uint32_t;
    int64_t     data_int32_t;
    uint64_t    data_uint64_t;
    float       data_float;
    double      data_double;
    void*       data_data;
    const void* data_data_const;
    char        *data_str;
    const char  *data_str_const;
}tlv_value;

typedef struct tlv_value_s {
    uint32_t  length;
    tlv_value value;
    bool is_deep;   /* 是否是深拷贝 */
    bool is_ptr;    /* 是否是指针 */
    bool is_takeover;   /* 是否接管内存释放 */
}tlv_value_t;

/**
 * @Brief  TLV的T，
 *         简化标准中的Tag结构，始终只占一个Byte。
 *     BIT7:
 *         0-> 表示数据是基本数据类型，即uint8_t uint16_t uint32_t等，在value中为深拷贝
 *         1-> 表示数据不是基本数据类型， 在value中为浅拷贝
 *     BIT6 - BIT0:
 *          对应的值表示对应的Tag值，因此一个TLV包结构最多包含128个TLV
 */
typedef struct tlv_tag_s
{
    uint8_t  is_ptr:1;
    uint8_t  tag:7;
}tlv_tag_t;

typedef struct tlv_s
{
    COBJ_HEAD_VARS;

    tlv_tag_t tag;
    uint32_t  length;
    tlv_value_t value;
}tlv_t;

typedef struct tlvp_s tlvp_t;

/**
 * @Brief  创建一个TLV包的唯一方式
 *
 * @Returns
 */
tlvp_t *tlvp_new(void);
/**
 * @Brief  释放由tlvp_new 创建的TLV包
 *
 * @Param tlvp
 */
void     tlvp_free(tlvp_t *tlvp);
uint32_t tlvp_cnt(const tlvp_t *tlvp);
bool     tlvp_contain(const tlvp_t *tlvp, uint8_t tag);
void     tlvp_get_value(const tlvp_t *tlvp, uint8_t tag, tlv_value_t *value);
int      tlvp_add(tlvp_t *tlvp, uint8_t tag, tlv_value_t *value);
/**
 * @Brief  计算tlvp_t在通过网络发送时所需要传送的数据大小
 *
 * @Param tlvp
 *
 * @Returns   对应的数据大小
 */
uint32_t tlvp_sizeof(const tlvp_t *tlvp);
void     tlvp_to_data(const tlvp_t *tlvp, void *data, uint32_t data_len);
void     tlvp_from_data(tlvp_t *tlvp, const void *data, uint32_t data_len);

#ifdef __cplusplus
}
#endif
#endif  /* CTLV_H_201409061909 */
