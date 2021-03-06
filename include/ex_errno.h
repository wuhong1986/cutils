#ifndef QJ_ERRNO_H_201310072010
#define QJ_ERRNO_H_201310072010
/*
 * =============================================================================
 *      Filename    :   qj_errno.h
 *      Description :   实现错误码的统一控制管理
 *      Created     :   2013-10-07 20:10:08
 *      Author      :    Wu Hong
 * =============================================================================
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short  Status;

#ifdef  S_OK
#undef S_OK
#endif

/*  系统错误 */
#define ERR_SYSTEM_START    0
#define ERR_SYSTEM_END      999
#define S_OK                            (ERR_SYSTEM_START + 0)
#define S_ERR                           (ERR_SYSTEM_START + 1)
#define ERR_PARAM_ERROR                 (ERR_SYSTEM_START + 2)
#define ERR_TTY_BAUDRATE_ERROR          (ERR_SYSTEM_START + 3)
#define ERR_TTY_SETATTR_FAILED          (ERR_SYSTEM_START + 4)
#define ERR_TTY_GETATTR_FAILED          (ERR_SYSTEM_START + 5)
#define ERR_TTY_STOPBITS_ERROR          (ERR_SYSTEM_START + 6)
#define ERR_TTY_PARITY_ERROR            (ERR_SYSTEM_START + 7)
#define ERR_TTY_DATABITS_ERROR          (ERR_SYSTEM_START + 8)
#define ERR_TTY_OPEN_FAILED             (ERR_SYSTEM_START + 9)
#define ERR_FILE_READ_TIMEOUT           (ERR_SYSTEM_START + 10)
#define ERR_LIST_NULL                   (ERR_SYSTEM_START + 11)
#define ERR_TTY_READ_FAILED             (ERR_SYSTEM_START + 12)
#define ERR_TTY_READ_TIMEOUT            (ERR_SYSTEM_START + 13)
#define ERR_FILE_OPEN_FAILED            (ERR_SYSTEM_START + 14)
#define ERR_FILE_SELECT_FAILED          (ERR_SYSTEM_START + 15)
#define ERR_DIR_EMPTY                   (ERR_SYSTEM_START + 16)
#define ERR_FILE_OR_DIR_NOT_EXIST       (ERR_SYSTEM_START + 17)
#define ERR_STATUS_PROC_FUN_NOT_FIND    (ERR_SYSTEM_START + 18)
#define ERR_FILE_WRITE_FAILED           (ERR_SYSTEM_START + 19)
#define ERR_FILE_READ_FAILED            (ERR_SYSTEM_START + 20)
#define ERR_DEV_TYPE_NOT_SUPPORT        (ERR_SYSTEM_START + 21)
#define ERR_PROGRESS_CANCELED           (ERR_SYSTEM_START + 22)
#define ERR_DISK_LOW                    (ERR_SYSTEM_START + 23)
#define ERR_CHECKSUM_ERROR              (ERR_SYSTEM_START + 24)
#define ERR_IOCTL_FAILED                (ERR_SYSTEM_START + 25)
#define ERR_NULL                        (ERR_SYSTEM_START + 26)
#define ERR_FILE_DESC_CLOSED            (ERR_SYSTEM_START + 27)
#define ERR_PRE_STATUS_NOT_OK           (ERR_SYSTEM_START + 28)
#define ERR_STATE_NOT_OK                (ERR_SYSTEM_START + 29)

#define ERR_SQL_ERR_START       1000
#define ERR_SQL_QUERY_FAILED    (ERR_SQL_ERR_START + 0)
#define ERR_SQL_DB_OPEN_FAILED  (ERR_SQL_ERR_START + 1)
#define ERR_SQL_DB_ALREADY_USED (ERR_SQL_ERR_START + 2)

#define ERR_DRIVER_ERR_START    1100
#define ERR_SPI_WRITE_FAILED    (ERR_DRIVER_ERR_START + 0)
#define ERR_SPI_READ_FAILED     (ERR_DRIVER_ERR_START + 1)

#define ERR_ALARM_PERF_START    1200
#define ERR_ALARM_PERF_CHECK_POINT_NOT_EXIST    (ERR_ALARM_PERF_START + 0)

#define ERR_INI_START               1300
#define ERR_INI_FORMAT_ERROR            (ERR_INI_START  + 0)
#define ERR_INI_CANNOT_FIND_SEC_END     (ERR_INI_START  + 1)
#define ERR_INI_CANNOT_FIND_EQUAL_CHAR  (ERR_INI_START  + 2)

#define ERR_CLI_START              1400
#define ERR_CLI_NO_SUCH_CMD     (ERR_CLI_START + 0)
#define ERR_CLI_CMD_FUNC_ERROR  (ERR_CLI_START + 1)
#define ERR_CLI_REMOTE_TIMEOUT  (ERR_CLI_START + 2)

#define ERR_SOCK_START              1500
#define ERR_SOCK_OPEN_FAILED        (ERR_SOCK_START + 1)
#define ERR_SOCK_BIND_FAILED        (ERR_SOCK_START + 2)
#define ERR_SOCK_WSA_STARTUP_FAILED (ERR_SOCK_START + 3)
#define ERR_SOCK_LISTEN_FAILED      (ERR_SOCK_START + 4)
#define ERR_SOCK_SET_OPT_FAILED     (ERR_SOCK_START + 5)
#define ERR_SOCK_CONNECT_FAILED     (ERR_SOCK_START + 6)

#define ERR_SEM_START           1700
#define ERR_SEM_CREATE_FAILED   (ERR_SEM_START + 1)
#define ERR_SEM_DOWN_TIMEOUT    (ERR_SEM_START + 2)
#define ERR_SEM_DOWN_FAILED     (ERR_SEM_START + 3)
#define ERR_SEM_UP_FAILED       (ERR_SEM_START + 4)

#define ERR_THREAD_START        1800
#define ERR_TH_LIST_FULL        (ERR_THREAD_START + 1)
#define ERR_TH_CREATE_FAILED    (ERR_THREAD_START + 2)
#define ERR_TH_NO_SUCH_ID       (ERR_THREAD_START + 3)

#define ERR_COMM_START      2000
#define ERR_COMM_NO_SUCH_ADDR               (ERR_COMM_START + 1)
#define ERR_COMM_ZIG_STATUS_CANNOT_POWER    (ERR_COMM_START + 2)
#define ERR_COMM_ZIG_SEND_AT_CMD_FAILED     (ERR_COMM_START + 3)
#define ERR_COMM_ZIG_NOT_JOIN_NETWORK       (ERR_COMM_START + 4)
#define ERR_COMM_ZIG_FRAME_READ_TIMEOUT     (ERR_COMM_START + 5)
#define ERR_COMM_ZIG_FRAME_READ_FAILED      (ERR_COMM_START + 6)
#define ERR_COMM_ZIG_FRAME_WRITE_TIMEOUT    (ERR_COMM_START + 7)
#define ERR_COMM_ZIG_FRAME_WRITE_FAILED     (ERR_COMM_START + 8)
#define ERR_COMM_ZIG_FRAME_CHECK_FAILED     (ERR_COMM_START + 9)
#define ERR_COMM_ZIG_FRAME_LEN_ERROR        (ERR_COMM_START + 10)
#define ERR_COMM_ZIG_FRAME_TX_ERR_CNT_OVER  (ERR_COMM_START + 11)
#define ERR_COMM_ZIG_NO_ROUTE_PATH          (ERR_COMM_START + 12)
#define ERR_COMM_ZIG_NO_ZIGBEE_DEV          (ERR_COMM_START + 13)
#define ERR_COMM_ZIG_IS_RESETING            (ERR_COMM_START + 14)

#define ERR_TLV_START                   2200
#define ERR_TLV_NO_SUCH_TAG             (ERR_TLV_START + 1)
#define ERR_TLV_DATA_LEN_NOT_ENOUGH     (ERR_TLV_START + 2)
#define ERR_TLV_TAG_ALREADY_EXIST       (ERR_TLV_START + 3)

#define ERR_CMD_START                   2300
#define ERR_CMD_ROUTINE_NOT_FOUND       (ERR_CMD_START + 1)
#define ERR_CMD_ROUTINE_ALREADY_EXIST   (ERR_CMD_START + 2)
#define ERR_CMD_REQ_TIMEOUT             (ERR_CMD_START + 3)
#define ERR_CMD_SEND_FAILED             (ERR_CMD_START + 4)
#define ERR_CMD_RECV_CHECKSUM_ERROR     (ERR_CMD_START + 5)
#define ERR_CMD_RECV_FAILED             (ERR_CMD_START + 6)
#define ERR_CMD_RESP_NO_SUCH_DEV        (ERR_CMD_START + 7)
#define ERR_CMD_TEST_DEV_STATUS_ERROR   (ERR_CMD_START + 8)
#define ERR_CMD_NO_SUCH_REQ_CMD         (ERR_CMD_START + 9)
#define ERR_CMD_START_BYTES_ERROR       (ERR_CMD_START + 10)
#define ERR_CMD_LENGTH_ERROR            (ERR_CMD_START + 11)
#define ERR_CMD_TASK_ALREAY_START       (ERR_CMD_START + 12)
#define ERR_CMD_REQ_NO_SUCH_ADDRID      (ERR_CMD_START + 13)
#define ERR_CMD_STATUS_ROUTINE_NOT_FOUND  (ERR_CMD_START + 14)
#define ERR_CMD_IS_NULL                 (ERR_CMD_START + 15)

#define ERR_ACQ_START                   2400
#define ERR_ACQ_ALREADY_START           (ERR_ACQ_START + 0)
#define ERR_ACQ_DSP_UPLOAD_FAILED       (ERR_ACQ_START + 1)
#define ERR_ACQ_GAIN_ERROR              (ERR_ACQ_START + 2)
#define ERR_ACQ_CS5376_READ_FAILED      (ERR_ACQ_START + 3)
#define ERR_ACQ_CS5376_WRITE_FAILED     (ERR_ACQ_START + 4)
#define ERR_ACQ_CS5376_WRITE_DF_FAILED  (ERR_ACQ_START + 5)
#define ERR_ACQ_CHANNEL_ERROR           (ERR_ACQ_START + 6)
#define ERR_ACQ_DATA_READ_TIMEOUT       (ERR_ACQ_START + 7)
#define ERR_ACQ_DATA_READ_ERROR         (ERR_ACQ_START + 8)
#define ERR_ACQ_DSP_CTRL_FAILED         (ERR_ACQ_START + 9)
#define ERR_ACQ_DSP_RESET_ON_FAILED     (ERR_ACQ_START + 10)
#define ERR_ACQ_DSP_RESET_OFF_FAILED    (ERR_ACQ_START + 11)
#define ERR_ACQ_DSP_READ_TIMEOUT        (ERR_ACQ_START + 12)
#define ERR_ACQ_DSP_WRITE_CHECK_FAILED  (ERR_ACQ_START + 13)
#define ERR_ACQ_DSP_APP_OPEN_FAILED     (ERR_ACQ_START + 14)
#define ERR_ACQ_DSP_APP_FORMAT_ERROR    (ERR_ACQ_START + 15)
#define ERR_ACQ_DSP_READ_CHECK_FAILED   (ERR_ACQ_START + 16)
#define ERR_ACQ_NOT_START               (ERR_ACQ_START + 17)
#define ERR_ACQ_START_TIME_IS_TOO_EARLY (ERR_ACQ_START + 18)
#define ERR_ACQ_STOP_TIMEOUT            (ERR_ACQ_START + 19)
#define ERR_ACQ_DSP_READ_ERROR          (ERR_ACQ_START + 20)
#define ERR_ACQ_GPS_NOT_LOCATED_EVER    (ERR_ACQ_START + 21)
#define ERR_ACQ_GPS_NOT_LEAPS           (ERR_ACQ_START + 22)
#define ERR_ACQ_START_TIME_NOT_CORRECT  (ERR_ACQ_START + 23)
#define ERR_ACQ_CS5376_SAMPLE_RATE      (ERR_ACQ_START + 24)
#define ERR_ACQ_CACHE_READ_OVER_RANGE   (ERR_ACQ_START + 25)
#define ERR_ACQ_ALREADY_STOP            (ERR_ACQ_START + 26)
#define ERR_ACQ_STOPED                  (ERR_ACQ_START + 27)
#define ERR_ACQ_READ_FAILED             (ERR_ACQ_START + 28)
#define ERR_ACQ_CACHE_READ_NOT_ENOUGH   (ERR_ACQ_START + 29)
#define ERR_ACQ_CACHE_WRITE_ONLY        (ERR_ACQ_START + 30)
#define ERR_ACQ_CPLD_WRITE_CHECK_FAILED (ERR_ACQ_START + 31)
#define ERR_ACQ_DEV_NOT_FOUND           (ERR_ACQ_START + 32)
#define ERR_ACQ_DEV_DISABLED            (ERR_ACQ_START + 33)
#define ERR_ACQ_FINISHED                (ERR_ACQ_START + 34)
#define ERR_ACQ_DEV_SYNC_FAILED         (ERR_ACQ_START + 35)

#define ERR_CALC_START                  2500
#define ERR_CALC_TASK_RUNNING           (ERR_CALC_START + 0)
#define ERR_CALC_TASK_STOPED            (ERR_CALC_START + 1)
#define ERR_CALC_READ_BUF_ERROR         (ERR_CALC_START + 2)
#define ERR_CALC_CS5376_DEV_PARAM_ERROR (ERR_CALC_START + 3)
#define ERR_CALC_STOP_TIMEOUT           (ERR_CALC_START + 4)
#define ERR_TASK_STOP_TIMEOUT           (ERR_CALC_START + 5)
#define ERR_TASK_NO_FUNC                (ERR_CALC_START + 7)
#define ERR_TASK_STOPED                 (ERR_CALC_START + 8)

#define ERR_GPS_START                   2600
#define ERR_GPS_STR_CHECK_FAILED        (ERR_GPS_START + 0)
#define ERR_GPS_STR_FORMAT_ERROR        (ERR_GPS_START + 1)
#define ERR_GPS_GPRMC_START_ERROR       (ERR_GPS_START + 2)
#define ERR_GPS_GPGGA_START_ERROR       (ERR_GPS_START + 3)
#define ERR_GPS_TTY_READ_FAILED         (ERR_GPS_START + 4)
#define ERR_GPS_TTY_DEV_OPEN_FAILED     (ERR_GPS_START + 5)
#define ERR_GPS_GET_ING                 (ERR_GPS_START + 6)
#define ERR_GPS_NOT_UPDATED             (ERR_GPS_START + 7)
#define ERR_GPS_NO_DATA                 (ERR_GPS_START + 8)

#define ERR_UPDATE_START                (2700)
#define ERR_UPDATE_SOFT_INFO_NOT_EXIST  (ERR_UPDATE_START + 0)
#define ERR_UPDATE_SOFT_INFO_NOT_MATCH  (ERR_UPDATE_START + 1)
#define ERR_UPDATE_SOFT_INFO_MATCH      (ERR_UPDATE_START + 2)
#define ERR_UPDATE_SOFT_NEED_UPDATE     (ERR_UPDATE_START + 3)
#define ERR_UPDATE_SOFT_SIZE_ERROR      (ERR_UPDATE_START + 4)
#define ERR_UPDATE_SOFT_RCVING          (ERR_UPDATE_START + 5)
#define ERR_UPDATE_SOFT_CHECK_OK        (ERR_UPDATE_START + 6)
#define ERR_UPDATE_SOFT_CHECK_FAILED    (ERR_UPDATE_START + 7)
#define ERR_UPDATE_SOFT_IDX_ERR         (ERR_UPDATE_START + 8)

#define ERR_TRANS_START                 (2800)
#define ERR_TRANS_ALREADY_START         (ERR_TRANS_START + 0)
#define ERR_TRANS_CMD_BYTES_NUM         (ERR_TRANS_START + 1)
#define ERR_TRANS_CMD_WRITE_CHECK       (ERR_TRANS_START + 2)

#define ERR_TEM_START                   (3000)
#define ERR_TEM_ACQ_ALREADY_START       (ERR_TEM_START + 1)
#define ERR_TEM_ACQ_BUF_READ_OVER_RANGE (ERR_TEM_START + 2)
#define ERR_TEM_ACQ_ALREADY_STOP        (ERR_TEM_START + 3)
#define ERR_TEM_ACQ_STOP_TIMEOUT        (ERR_TEM_START + 4)
#define ERR_TEM_ACQ_STOPED              (ERR_TEM_START + 5)
#define ERR_TEM_ACQ_READ_FAILED         (ERR_TEM_START + 6)
#define ERR_TEM_ACQ_DATA_OFFSET_ERROR   (ERR_TEM_START + 7)
#define ERR_TEM_ACQ_WAITING             (ERR_TEM_START + 8)
#define ERR_TEM_ACQ_NEED_CONTINUE       (ERR_TEM_START + 9)
#define ERR_TEM_ACQ_IDX_ERROR           (ERR_TEM_START + 10)
#define ERR_TEM_CPLD_BYPF1_ERR          (ERR_TEM_START + 11)
#define ERR_TEM_CPLD_BYPF3_ERR          (ERR_TEM_START + 12)
#define ERR_TEM_CPLD_DEC_ERR            (ERR_TEM_START + 13)
#define ERR_TEM_TRANS_CURR_LENGTH       (ERR_TEM_START + 14)
#define ERR_TEM_CURR_DATA_IS_ACQING     (ERR_TEM_START + 15)
#define ERR_TEM_TRANS_CFG_FAILED        (ERR_TEM_START + 16)
#define ERR_TEM_TRANS_CURR_DATA_TIMEOUT (ERR_TEM_START + 17)
#define ERR_TEM_TRANS_START_FAILED      (ERR_TEM_START + 18)
#define ERR_TEM_TRANS_STOP_FAILED       (ERR_TEM_START + 19)
#define ERR_TEM_GPS_SYNC_WAITING        (ERR_TEM_START + 20)
#define ERR_TEM_ACQ_START_FAILED        (ERR_TEM_START + 30)
#define ERR_TEM_TRANS_CURR_DATA_TYPE_ERR (ERR_TEM_START + 31)
#define ERR_TEM_TRANS_CMD_ADDR_MISMATCH (ERR_TEM_START + 32)

#define ERR_GSA12_START                 (3200)
#define ERR_GSA12_CHANNEL_NUM_ERROR     (ERR_GSA12_START + 1)

#define ERR_PIP_PROC_START              (20000)
#define ERR_PIP_PROC_DATA_OVERFLOW      (ERR_PIP_PROC_START + 1)
#define ERR_PIP_PROC_DATA_INVALID       (ERR_PIP_PROC_START + 2)
#define ERR_PIP_PROC_DATA_POWER_IN_MN   (ERR_PIP_PROC_START + 3)
#define ERR_PIP_PROC_DATA_N_GT_70       (ERR_PIP_PROC_START + 4)
#define ERR_PIP_PROC_DATA_MN_NOT_MULTI_N (ERR_PIP_PROC_START + 5)
#define ERR_PIP_PROC_POLE3_NEGTIVE      (ERR_PIP_PROC_START + 6)
#define ERR_PIP_PROC_POLE3_POSTIVE      (ERR_PIP_PROC_START + 7)
#define ERR_PIP_PROC_AMP_OVERFLOW       (ERR_PIP_PROC_START + 8)
#define ERR_PIP_PROC_CP_OVERFLOW        (ERR_PIP_PROC_START + 9)
#define ERR_PIP_PROC_FS_OVERFLOW        (ERR_PIP_PROC_START + 10)
#define ERR_PIP_PROC_DATA_MN_GT_800     (ERR_PIP_PROC_START + 11)
#define ERR_PIP_PROC_AC_OVERFLOW        (ERR_PIP_PROC_START + 12)
#define ERR_PIP_PROC_NOT_IN_GRID        (ERR_PIP_PROC_START + 13)

#define ERR_PRINTER_START                   (30000)
#define ERR_PRINTER_PRN_SEND_FINISH         (ERR_PRINTER_START + 0)
#define ERR_PRINTER_PRN_SEND_WAIT           (ERR_PRINTER_START + 1)
#define ERR_PRINTER_NO_SUCH_TASK            (ERR_PRINTER_START + 2)
#define ERR_PRINTER_TASK_BUSY               (ERR_PRINTER_START + 3)
#define ERR_PRINTER_NO_SUCH_BOARD           (ERR_PRINTER_START + 4)
#define ERR_PRINTER_FPGA_WRITE_FAILED       (ERR_PRINTER_START + 5)
#define ERR_PRINTER_FPGA_READ_FAILED        (ERR_PRINTER_START + 6)
#define ERR_PRINTER_FPGA_WRITE_CHK_FAILED   (ERR_PRINTER_START + 7)

typedef void (*cb_err_msg_free)(void *msg);

/**
 * @Brief  添加一个错误码
 *
 * @Param err 错误码
 * @Param desc  错误描述
 */
void ex_err_add(Status err_code, const char *desc);
/**
 * @Brief  获取错误码的错误描述
 *
 * @Param err
 *
 * @Returns
 */
const char* ex_err_str(Status err_code);
void  ex_err_init(void);
void  ex_err_release(void);

#ifdef __cplusplus
}
#endif
#endif  /* QJ_ERRNO_H_201310072010 */
