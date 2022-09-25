#include "sm_main.h"
#include "sm_system.h"
#include "sm_debug.h"
#include "sm_main.h"


//MTK头文件
#include "ReminderSrvTypeTable.h"
#include "custom_mmi_default_value.h"
#include "ReminderSrvGprot.h"
#include "TimerEvents.h"
#include "gpiosrvgprot.h"
#include "BootupSrvGprot.h"
#include "l4c_common_enum.h"

char version_str[10] = {0};

static const sm_u16 day_of_month[2][12+1] =
{
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


void sm_bcd_2_number_string(sm_u8 length, sm_u8 *bcd_ptr, sm_u8 *digital_ptr)
{
    sm_s32         i;
    sm_u8         temp;

    // get the first digital
    temp = (sm_u8)((*bcd_ptr >> 4) &0x0f);
    if (temp >= 0x0a)
    {
        *digital_ptr = (sm_u8)((temp - 0x0a) + 'A');
    }
    else
    {
        *digital_ptr = (sm_u8)(temp + '0');
    }

    bcd_ptr++;
    digital_ptr++;
    for (i=0; i<(length - 1); i++)
    {
        temp = *bcd_ptr;
        temp &= 0x0f;
        if (temp >= 0x0a)
        {
            *digital_ptr = (sm_u8)((temp - 0x0a) + 'A');
        }
        else
        {
            *digital_ptr = (sm_u8)(temp + '0');
        }
        digital_ptr++;
        temp = *bcd_ptr;
        temp = (sm_u8)((temp & 0xf0) >> 4);
        if ((temp == 0x0f) && (i == (length -1)))
        {
            *digital_ptr = '\0';
            return;
        }
        else if (temp>=0x0a)
        {
            *digital_ptr = (sm_u8)((temp - 0x0a) + 'A');
        }
        else
        {
            *digital_ptr = (sm_u8)(temp + '0');
        }
        digital_ptr++;
        bcd_ptr++;
    }
    *digital_ptr = '\0';
}

sm_u32 sm_sys_get_check_updata_freq()
{
	return 12*60*60*1000;
}

/**
函数说明：判断SIM是否已经插入
参数说明：无
返回值：SM_TRUE表示有效，SM_FALSE表示无效 
*/
sm_bool sm_sys_simcard_insert()
{
	return SM_TRUE;
}

/**
函数说明：判断SIM是否可以识别
参数说明：无
返回值：SM_TRUE表示可以识别，SM_FALSE表示不能识别
*/
sm_bool sm_sys_simcard_recognize()
{
	return SM_TRUE;
}

/**
函数说明：判断imei是否已经写入
参数说明：无
返回值：SM_TRUE表示已经写入，SM_FALSE表示没有写入
*/
sm_bool sm_sys_imei_valid()
{
	return SM_TRUE;
}

/******************************************************************************************************/
/*
函数说明：系统相关功能初始化
参数说明：无
返回值：成功返回SM_ERR_OK
*/
sm_s32 sm_sys_init()
{
	int errCode = SM_ERR_OK;

	sm_soc_init();

	return errCode;
}

/*
函数说明：重启进入升级模式
特别说明：在其他的平台需要实现这个接口，windows模拟环境不需要实现
*/
void sm_sys_reboot()
{
	// 重启进入升级模式
	srv_reboot_normal_start();
	SM_PORITNG_LOG("sm_sys_reboot ..");
}


/*
函数说明：获取产品ID
参数说明：buf 返回产品ID
*/
char *sm_sys_get_product_id()
{
	return "1537863961522";
}

/*
函数说明：获取设备ID
参数说明：imei 返回设备ID 
*/
sm_bool sm_sys_get_mid(char imei[16], sm_u32 bl)
{
	// return IMEI
#if defined(WIN32)
	strcpy(imei, "123456789012343"); //"1000036A6B7557" // "1000036A6B9750"
	return KAL_TRUE;
#else	
	return srv_imei_get_imei(MMI_SIM1, imei, bl);
#endif
}


/*
函数说明：获取基站数据
参数说明： 
*/
void sm_sys_get_gsm_info(sm_gsm_info *info)
{
	info->mcc = 460;
	info->mnc = 0;
	info->lac = 0;
	info->cid = 0;
}

/**********************************************************************************************
NAME	 : sm_restart
FUCNTION : 模组重启
PARAMETER:
RETURN	 :
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
void sm_restart()
{
	srv_reboot_normal_start();
}

/**********************************************************************************************
NAME	 : sm_power_off
FUCNTION : 模组关机
PARAMETER:
RETURN	 :
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
void sm_power_off()
{
	srv_shutdown_normal_start(0);
}

/**********************************************************************************************
NAME	 : sm_get_version
FUCNTION : 获取版本号
PARAMETER:
RETURN	 :
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
char *sm_get_version()
{
	memset(version_str, 0x00, 10);
	kal_snprintf(version_str, 10, "%s", release_verno());
	return version_str;
}

/*
函数说明：	获取UTC时间戳
参数说明：
返回值：	UTC时间戳
*/
sm_u32 sm_get_utc_seconds(void)
{
	applib_time_struct rtc, utc;
	
	applib_dt_get_date_time((applib_time_struct*)&rtc);
	applib_dt_rtc_to_utc_with_default_tz((applib_time_struct *)&rtc, (applib_time_struct *)&utc);
	return applib_dt_mytime_2_utc_sec(&utc,MMI_FALSE);
}


