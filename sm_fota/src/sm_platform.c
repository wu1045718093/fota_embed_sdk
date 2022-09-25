#include "sm_datatype.h"
#include "sm_platform.h"
#include "sm_thread.h"
#include "sm_debug.h"
#include "string.h"
#include "stdarg.h"
#include "stdlib.h"
#include "MMI_features.h"
#include "MMI_include.h"
#include "med_utility.h"


void * g_msg_handle = SM_NULL;
static const sm_s8 dec2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};


const sm_s8* sm_strstr(const sm_s8* src, const sm_s8* sub)
{
	return strstr(src, sub);
}

sm_s32 sm_vsnprintf(sm_s8 * buf, sm_u32 bufSize, const sm_s8 * format, va_list arglist)
{
	return vsnprintf(buf, bufSize, format, arglist);
}

sm_s8* sm_strcpy(char* dest, const char* src)
{
	return strcpy(dest, src);
}

sm_s8* sm_strcat(char* dest, const char* src)
{
	return strcat(dest, src);
}

sm_u32 sm_strlen(const char* src)
{
	return strlen(src);
}

sm_s32 sm_strcmp(const char* str1, const char* str2)
{
	return strcmp(str1, str2);
}

const sm_s8* sm_strchr(const sm_s8* src, int chr)
{
	return strchr(src, chr);
}

void* sm_memcpy(void* dest, const void* src, sm_u32 size)
{
	return memcpy(dest, src, size);
}

void* sm_memset(void * dest, sm_s32 val, sm_u32 size)
{
	return memset(dest, val, size);
}

sm_s32 sm_memcmp(const void* src, const void* dest, sm_s32 size)
{
	return memcmp(src, dest, size);
}


sm_s32 sm_sprintf(sm_s8 * buf, const char * format, ...)
{
    sm_s32 ret;
    va_list args ;

    va_start (args, format) ;
    ret = vsprintf (buf, format, args) ;
    va_end (args) ;

    return ret;
}

sm_s32 sm_atoi(const char* str)
{
	return atoi(str);
}

void* sm_malloc(sm_u32 allocSize)
{
	void *returnPointer;
	
	returnPointer = (void *)med_alloc_ext_mem(allocSize);

	return(returnPointer);
}

void  sm_mfree(void* memBlock)
{
	med_free_ext_mem((void**)&memBlock);
}



 

/**
函数说明：输入日志到文件或者其他
参数说明：需要输出的日志内容
返回值：无
*/
void sm_cb_printLog(const sm_s8* msg)
{
	if(0 == msg) 
		return;

	//kal_prompt_trace(msg); 
}



/*
函数说明：	启动定时器
参数说明：
			timer_id:		定时器ID	0-16
			timer_expire:	定时器的定时时间		单位ms
			timer_func:		定时器的回调函数
返回值：
			0-失败
			其它-成功
注意:		使用该函数请先在头文件里定义一个唯一的定时器ID

*/
sm_s32 wt_start_timer(sm_u32 timer_id,
						  sm_u32 timer_expire,
						  FuncPtr callback)
{ 
	StartTimer(timer_id, timer_expire, (FuncPtr)callback);	
}

/*
函数说明： 启动定时器
参数说明：
			timer_id:		定时器ID 0-16
			timer_expire:	定时器的定时时间		单位ms
			timer_func: 	定时器的回调函数
			user_data:		回调函数参数
返回值：
			0-失败
			其它-成功
注意: 	使用该函数请先在头文件里定义一个唯一的定时器ID

*/
sm_s32 wt_start_timer_ex(sm_u32 timer_id,
							 sm_u32 timer_expire,
							 FuncPtr callback,
							 void *user_data)
{
	StartTimerEx(timer_id, timer_expire, (FuncPtr)callback, user_data);	
}



void wt_stop_timer(sm_u32 timer_id)
{
	StopTimer(timer_id);
}


