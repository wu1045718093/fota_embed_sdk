#ifndef __SM_MAIN_H__
#define __SM_MAIN_H__

#include "sm_socket.h"

#define SM_BUF_SIZE        2048

typedef void (*sm_callback) (char state, int percent);

typedef enum {
	SMDT_STATE_IDLE = 0,
	SMDT_STATE_READY,
	SMDT_STATE_DODNS,      // 正在做DNS解析
	SMDT_STATE_DNSDONE,    // 域名解析完毕
	SMDT_STATE_SOCOPEN,    // socket已创建
	SMDT_STATE_CONNTING,   // 正在连接socket
	SMDT_STATE_ESTABLISH,  // 连接已建立
    SMDT_STATE_SEND_REQ,   // 发送POST/GET请求
    SMDT_STATE_WAIT_ACK,   // 等待POST/GET回复
    SMDT_STATE_TRANSING,   // 文件数据块传输
    SMDT_STATE_WAIT_FINISH,// 等待传输结束信号
	SMDT_STATE_ERROR,	   // 出错状态
} SMDT_STATE_E;

typedef enum {
	SM_DL_CHECK_ERROR = 0,
	SM_DL_CHECK_NO_UPDATE,
	SM_DL_CHECK_DOWNLOADING,
	SM_DL_CHECK_DOWNLOAD_ERROR,
} SM_DL_STATE_E;

typedef struct {
	sm_u32 buf_len;					//buf长度
	sm_u32 bin_len;					//bin文件的总大小
	sm_u32 cur_len;					//当前已下载的大小
    char buffer[SM_BUF_SIZE];       //读写缓冲
   	char mode;						//连接类型		 0--HTTP	1--HTTPS
    SMDT_STATE_E state;				//下载状态
} sm_http_t;

typedef struct {
	// reference: global_cell_id_struct (nbr_public_struct.h)
    sm_u16 mcc;
    sm_u16 mnc;
    sm_u16 lac;
    sm_u16 ci;
	
	// reference: gas_nbr_cell_meas_struct (nbr_public_struct.h)
    sm_u16 arfcn;
    sm_u8  bsic;
    sm_u8  rxlev;
} SM_GSM_CELL_INFO;

typedef struct {
	SM_GSM_CELL_INFO cell;
	sm_s16        rssi;
	char         plmn[6];
	sm_u8        rssi_lev;
} SM_SRV_CELL_INFO;

typedef struct {
	SM_GSM_CELL_INFO nbr[6];
	SM_SRV_CELL_INFO srv;
	sm_u8  nbr_num;
} SM_NBR_CELL_INFO;

void sm_soc_connect_timeout(void *data);
sm_s32 sm_protocol_connect(sm_socket_handle soc_id);
sm_s32 sm_protocol_read(sm_socket_handle soc_id);
sm_s32 sm_protocol_write(sm_socket_handle soc_id);
sm_s32 sm_protocol_close(sm_socket_handle soc_id);
sm_s32 sm_protocol_dns(sm_u32 ip_addr, sm_u32 request_id, int error_code);
void sm_soc_init();
void sm_sdk_autoCheck();
void sm_download_report(const char *state, sm_u8 persent, sm_s8 error_code);
mmi_ret sm_event_handler(mmi_event_struct *evt);


#endif
