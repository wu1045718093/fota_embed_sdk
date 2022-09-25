#ifndef __SM_THREAD_H__
#define __SM_THREAD_H__

#include "sm_datatype.h"

//sdk需要处理的消息
#define UA_TASK_RUN_SIGNAL_CODE  ((0xF1 << 8) | 1)    
// 自动检测周期到来需要发送的消息，网络激活之后也需要调用此消息
#define UA_TASK_AUTO_CHECK_TIMER_COME_SIGNAL_CODE  ((0xF2 << 8) | 1)  
// 启动检测的消息
#define UA_TASK_CHECK_SIGNAL_CODE ((0xF3 << 8) | 1)
// 启动下载的消息
#define UA_TASK_DOWNLOAD_SIGNAL_CODE ((0xF4 << 8) | 1)
// 启动升级的消息
#define UA_TASK_INSTALL_SIGNAL_CODE ((0xF5 << 8) | 1)
// 启动网络监控定时器
#define UA_TASK_CHECK_NETWORK_SIGNAL_CODE ((0xF6 << 8) | 1)
// 网络可用
#define UA_TASK_NETWORK_CAN_USE_SIGNAL_CODE ((0xF7 << 8) | 1)
// 取消网络监控定时器
#define UA_TASK_CHECK_NETWORK_TIMER_COME_SIGNAL_CODE ((0xF8 << 8) | 1)
// 修改自动检测周期为正常周期
#define UA_TASK_CHANGE_AUTOCHECK_TIMER_SIGNAL_CODE ((0xF9 << 8) | 1)
// timer message
#define UA_TASK_TIMER_MSG_SIGNAL_CODE ((0xFA << 8) | 1)

sm_s32  sm_create_thread();
void sm_post_message_to_thread_with_code(unsigned short signalCode,  void* msgData);
sm_s32  tmp_NetworkCanUse();
#endif