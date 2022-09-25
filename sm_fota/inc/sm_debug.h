#ifndef __SM_DEBUG_H__
#define __SM_DEBUG_H__

#include "sm_datatype.h"
#include <stdarg.h>

#ifdef __cplusplus
  extern "C"
  {
#endif


/**
函数说明：打印log日志 
参数说明：参见printf的实现
返回值：VOID
*/
void sm_print_log(const sm_s8* modelTag, const sm_s8* line, sm_s8* fmt, va_list arglist);


void sm_platform_print_log(sm_s8* fmt, ...);

//void sm_platform_new_print_log(sm_s8* fmt, ...);



/*
1.DEBUG : 
DEBUG Level指出细粒度信息事件对调试应用程序是非常有帮助的。 
2. INFO 
INFO level表明 消息在粗粒度级别上突出强调应用程序的运行过程。 
3.WARN 
WARN level表明会出现潜在错误的情形。 
4.ERROR 
ERROR level指出虽然发生错误事件，但仍然不影响系统的继续运行。 
5.FATAL 
FATAL level指出每个严重的错误事件将会导致应用程序的退出。
*/

#define SM_LOG_FATAL  "<1>"
#define SM_LOG_ERROR  "<2>"
#define SM_LOG_WARN	  "<3>"
#define SM_LOG_INFO   "<4>"
#define SM_LOG_DEBUG  "<sm_thread>"

#define SMLOG_FATAL 1
#define SMLOG_ERROR 2
#define SMLOG_WARN  3
#define SMLOG_INFO  4
#define SMLOG_DEBUG 5

#define SM_LOG_DEFAULT "<4>" //默认级别为4

#define SM_DEFAULT_LEVEL 4

//#define PRINT_LOG
#define SM_PORITNG_LOG(fmt,arg...)   kal_prompt_trace(MOD_BT, fmt, ##arg)
//#define SM_PORITNG_LOG			dbg_print

#ifdef __cplusplus
  }
#endif

#endif
