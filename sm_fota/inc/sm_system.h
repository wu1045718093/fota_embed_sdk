#ifndef __SM_SYSTEM_H__
#define __SM_SYSTEM_H__

#include "sm_datatype.h"
#include "sm_error.h"
#include "sm_platform.h"


//MTK头文件
#include "MMI_features.h"
#include "MMI_include.h"
#include "TimerEvents.h"

#define SM_OTA_CID 7
#define SM_AUTO_CHECK_FREQ 		(3*60*60*1000)


sm_bool sm_sys_simcard_insert();
sm_bool sm_sys_simcard_recognize();
sm_bool sm_sys_imei_valid();
sm_s32 sm_sys_init();
void sm_sys_get_time(sm_s8* buf);
void sm_sys_get_time_ex(sm_s8* buf);
void sm_sys_reboot();
void sm_power_off();
sm_u32 sm_sys_get_check_updata_freq();
char *sm_sys_get_product_id();


enum
{
	SM_NETWORK_CHECK_TIMER = TIMER_ID_WT_FOTA_TIMER_BASE,
	SM_AUTO_CHECK_UPDATA,
	SM_CONNECT_TIMER_ID,
	SM_TLS_HANDSHAKE_TIMER_ID,
	SM_HTTP_CONNECT_TIMER_ID,
	SM_HTTP_HANDSHAKE_TIMER_ID,
	SM_HTTP_RETRY_TIMER_ID,
	SM_RESTART_TIMER_ID
};

typedef struct
{
	sm_u16 mcc;
	sm_u16 mnc;
	sm_u16 lac;
	sm_u16 cid;
} sm_gsm_info;


void sm_restart();
sm_u32 sm_get_utc_seconds(void);

#endif
