#include <time.h>
#include <errno.h>

#include "sm_datatype.h"
#include "sm_system.h"
#include "sm_thread.h"
#include "sm_debug.h"
#include "sm_socket.h"
#include "sm_platform.h"
#include "sm_main.h"

// 6531相关头文件
#include "os_api.h"
#include "nv_item_id.h"
#include "mn_type.h"
#include "nvitem.h"
#include "version.h"
#include "sci_service.h"
#include "mn_events.h"
#include "sig_code.h"
#include "sci_api.h"
#include "IN_Message.h"
#include "Dal_time.h"
#include "dal_power.h"


BLOCK_ID s_uadl_thread_id = SCI_INVALID_BLOCK_ID;    // 线程ID
#define UADL_OS_THREAD_STACK_SIZE (50*1024)     // 线程占用的堆栈大小，设置不合理可能会引发异常如assert
#define s_uadl_thread_task_priority     41	//线程优先级，20为普通优先级
#define UA_OS_THREAD_QUEUE_NUM    20    //线程消息队列数

LOCAL sm_s32 sm_wait_network_active_mag = 0;


//LOCAL sm_s32 g_network_cost_time = 0;
LOCAL sm_s32 g_tmp_network_state = 0;


LOCAL void sm_thread_register_event(void);


typedef struct SM_SIG_MSG_tag
{
	xSignalHeaderRec header; 
	void* msgDataEx;
} SM_SIG_MSG;

void sm_active_pdp_timeout()
{
	sm_wait_network_active_mag = 0;
	sm_system_ppp_deactive();
	sm_system_ppp_deattach();
}

void sm_auto_check_updata()
{
	sm_u32 timer;

	timer = sm_sys_get_check_updata_freq();
	if (sm_system_ppp_active_state())
	{
		SM_PORITNG_LOG(SM_LOG_DEBUG"sm_auto_check_updata, network can use\n");
		sm_sdk_autoCheck();
	}
	else
	{
		if (sm_system_is_ppp_attached())
		{
			if(sm_system_ppp_active())
			{
				sm_wait_network_active_mag = 1;
				sm_start_timer(SM_NETWORK_CHECK_TIMER, 120*1000, sm_active_pdp_timeout);
			}
			else
			{
				SM_PORITNG_LOG(SM_LOG_DEBUG"active fail\n");
			}
		}
		else
		{
			if(sm_system_ppp_attach())
			{
				sm_wait_network_active_mag = 1;
				sm_start_timer(SM_NETWORK_CHECK_TIMER, 120*1000, sm_active_pdp_timeout);
			}
			else
			{
				SM_PORITNG_LOG(SM_LOG_DEBUG"attach fail\n");
			}
		}
	}

	//启动一个定时器定时检测更新
	sm_start_timer(SM_AUTO_CHECK_UPDATA, timer, sm_auto_check_updata);
}


// 检查下载的执行入口
LOCAL void Ua_Task_Entry(uint32 argc, void *argv)
{
	SM_SIG_MSG *psig;
	unsigned short signalCode;

	sm_thread_register_event();

	while(1)
	{			
		void* msgData = 0;
		psig = SCI_NULL;

		SCI_RECEIVE_SIGNAL((xSignalHeader)psig,  s_uadl_thread_id);	 //阻塞获取，等待定时器发送
		signalCode = psig->header.SignalCode;
		msgData = psig->msgDataEx;

		//SM_PORITNG_LOG("[sm_thread] recv msg, signalCode = 0X%X\n", signalCode);

		switch(signalCode)
		{
			case APP_MN_ACTIVATE_PDP_CONTEXT_CNF:
			{
				SM_PORITNG_LOG("[sm_thread] recv APP_MN_ACTIVATE_PDP_CONTEXT_CNF");
				if (sm_wait_network_active_mag == 1)
				{
					sm_wait_network_active_mag = 0;
					sm_stop_timer(SM_NETWORK_CHECK_TIMER);
					
					if((((APP_MN_GPRS_EXT_T*)psig)->result >= MN_GPRS_ERR_SUCCESS) && (((APP_MN_GPRS_EXT_T*)psig)->result <= MN_GPRS_ERR_SUCCESS_SINGLE_ONLY))
					{
						APP_MN_GPRS_EXT_T*param_ptr = (APP_MN_GPRS_EXT_T *)psig;
						uint8 nsapi = 5;
						uint8 pdp_id = 1;
				
						nsapi = param_ptr->nsapi;
						pdp_id = param_ptr->pdp_id;

						sm_socket_set_netid(nsapi);

						//调用检测更新接口
						sm_sdk_autoCheck();
					}
					else
					{
						SM_PORITNG_LOG("[sm_thread] recv network active fail message\n");
					}
				}
				else
				{
					SM_PORITNG_LOG("[sm_thread] recv network active message 2, but no need deal\n");
				}
			}
			
			case APP_MN_GPRS_ATTACH_RESULT:
			{
				SM_PORITNG_LOG("[sm_thread] recv APP_MN_GPRS_ATTACH_RESULT");
				if (sm_wait_network_active_mag == 1)
				{
					APP_MN_GPRS_ATTACH_RESULT_T*param_ptr = (APP_MN_GPRS_ATTACH_RESULT_T *)psig;
					
					sm_wait_network_active_mag = 0;
					sm_stop_timer(SM_NETWORK_CHECK_TIMER);
					
					if(param_ptr->is_attach_ok) 
					{
						if(sm_system_ppp_active())
						{
							sm_wait_network_active_mag = 1;
							sm_start_timer(SM_NETWORK_CHECK_TIMER, 30*1000, sm_active_pdp_timeout);
						}
					}
					else
					{
						SM_PORITNG_LOG("[sm_thread] recv network attach fail message\n");
					}
				}
				else
				{
					SM_PORITNG_LOG("[sm_thread] recv network active message 2, but no need deal\n");
				}

				break;
			}
			case APP_MN_NETWORK_STATUS_IND:
			{
				//SM_PORITNG_LOG("[sm_thread] APP_MN_NETWORK_STATUS_IND");
				break;
			}
			case UA_TASK_TIMER_MSG_SIGNAL_CODE:
			{
				SM_SIG_TIMER_MSG *timer = (SM_SIG_TIMER_MSG *)psig;
				SM_PORITNG_LOG("[sm_thread] UA_TASK_TIMER_MSG_SIGNAL_CODE");
				sm_timer_message_handle(timer);
				break;
			}
			case APP_MN_PS_POWER_ON_CNF:
			{
				//开机之后120s检测是否有更新
				sm_start_timer(SM_AUTO_CHECK_UPDATA, 60*1000, sm_auto_check_updata);
				break;
			}
	        case SOCKET_ASYNC_GETHOSTBYNAME_CNF:
	        case SOCKET_CONNECT_EVENT_IND:
	        case SOCKET_READ_EVENT_IND:
	        case SOCKET_WRITE_EVENT_IND:
	        case SOCKET_CONNECTION_CLOSE_EVENT_IND:
	            {
					SM_PORITNG_LOG("[sm_thread] SOCKET_EVENT_IND\n");
					sm_soc_message_dispatch(psig);
	                break;
	            }
			default:
	            //MI_IOT_LOG("[MI_MAIN]: other Signal Code (%d)!", sig_ptr->SignalCode);
	            break;
		}
		
		SCI_FREE_SIGNAL(psig);
	}
}

sm_s32  sm_create_thread()
{
	s_uadl_thread_id = SCI_INVALID_BLOCK_ID;
	s_uadl_thread_id = SCI_CreateAppThread("T_UA_DL", "Q_SM_DL", Ua_Task_Entry, 0, NULL, UADL_OS_THREAD_STACK_SIZE, 
		UA_OS_THREAD_QUEUE_NUM, s_uadl_thread_task_priority, SCI_PREEMPT, SCI_AUTO_START);

	if(SCI_INVALID_BLOCK_ID == s_uadl_thread_id)
	{
		SM_PORITNG_LOG("[sm_thread] create app thread failed\n");
	}
}


void sm_post_message_to_thread_with_code(unsigned short signalCode,  void* msgData)
{
	SM_SIG_MSG* sig_ptr;
	BLOCK_ID dest_id;
	uint32 status;

	if(SCI_INVALID_BLOCK_ID == s_uadl_thread_id)
	{
		//SM_PORITNG_LOG(SM_LOG_DEBUG"message thread not exit\n");
		return;
	}

	// 创建消息，并赋值
	sig_ptr = SCI_ALLOC(sizeof(SM_SIG_MSG));
	SCI_ASSERT(SCI_NULL != sig_ptr);
	sig_ptr->header.SignalCode = signalCode;
	sig_ptr->header.SignalSize = sizeof(SM_SIG_MSG);
	sig_ptr->header.Sender = 0;
	sig_ptr->msgDataEx = msgData;
	/*
	if(SCI_SUCCESS != SCI_IsThreadQueueAvilable(s_uadl_thread_id))
	{
	SM_PORITNG_LOG(SM_LOG_DEBUG"ua timer send signalCode failed signalCode = %d,queue is full", signalCode);	
	return;
	}
	*/
	// 发送消息
	status = SCI_SendSignal((xSignalHeader)sig_ptr, s_uadl_thread_id);
	if (SCI_SUCCESS != status)
	{
		// 发送失败，进行出错处理
		//SM_PORITNG_LOG(SM_LOG_DEBUG"send msg fail, code= %d", signalCode);	
	}

}

sm_s32  tmp_NetworkCanUse()
{
	if (g_tmp_network_state == 0)
		return FALSE;
	else
		return TRUE;
}


BLOCK_ID sm_get_thread_id()
{
	return s_uadl_thread_id;
}



LOCAL void sm_thread_register_event(void)
{
    POWER_RESTART_CONDITION_E   start_condition = POWER_GetRestartCondition();

    if ((RESTART_BY_ALARM != start_condition && RESTART_BY_CHARGE != start_condition)
        || 1 == CLASSMARK_GetModelType() )
	{
	        //Register phone event
	        SCI_RegisterMsg( MN_APP_PHONE_SERVICE, 
	                                (uint8)EV_MN_APP_NETWORK_STATUS_IND_F, 
	                                (uint8)(MAX_MN_APP_PHONE_EVENTS_NUM -1), 
	                                SCI_NULL );

	        SCI_RegisterMsg(MN_APP_GPRS_SERVICE,
	                                (uint8)EV_MN_APP_SET_PDP_CONTEXT_CNF_F,
	                                (uint8)(MAX_MN_APP_GPRS_EVENTS_NUM - 1),
	                                SCI_NULL); 
	}

}
