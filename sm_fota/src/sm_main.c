#include "sm_main.h"
#include "sm_system.h"
#include "sm_socket.h"
#include "sm_debug.h"
#include "sm_download.h"
#include "sm_flash.h"
#include "3DES.h"
#include "NwInfoSrvGprot.h"
#include "BootupSrvGprot.h"
#include "ShutdownSrvGprot.h"
#include "nbr_public_struct.h"
#include "NwInfoSrv.h"
//#include "mmi_rp_app_sm_fota_def.h"


#define SM_FOTA_PORT	80


static sm_socket_list_struct sm_soc = {0};
static sm_http_t sm_http_info = {0};
sm_socket_list_struct *p_sm_soc = &sm_soc;
sm_http_t *p_sm_http_info = &sm_http_info;
static sm_u8 wait_ack_clean_ota_status = 0;
static unsigned char des3_key[] = "79071f151929cedf51da4640d392bf437c9e4f8f2fb34334";
static unsigned char sm_fota_host[] = "fota.anruii.com";

#if 1//NBR cell infomation
static SM_NBR_CELL_INFO mNbrCellInfo = {0};
void SNNbrCellInfoGetFunClr(void)
{	
	ClearProtocolEventHandler(MSG_ID_L4C_NBR_CELL_INFO_IND);
	ClearProtocolEventHandler(MSG_ID_L4C_NBR_CELL_INFO_REG_CNF);
	mmi_frm_send_ilm(MOD_L4C, MSG_ID_L4C_NBR_CELL_INFO_DEREG_REQ, NULL, NULL);
}
static void _sn_cell_nbr_rsp(l4c_nbr_cell_info_ind_struct *msg_ptr)
{
	gas_nbr_cell_info_struct *nci;
	
	nci = &msg_ptr->ps_nbr_cell_info_union.gas_nbr_cell_info;
	if (msg_ptr->is_nbr_info_valid)
	{
		kal_int32 i;
		gas_nbr_cell_meas_struct *meas;
		srv_nw_info_cntx_struct  *cntx;
		
		cntx = srv_nw_info_get_cntx(MMI_SIM1);
		mNbrCellInfo.srv.rssi = cntx->signal.gsm_RSSI_in_qdBm;
		mNbrCellInfo.srv.rssi_lev = cntx->signal.strength_in_percentage;
		strcpy(mNbrCellInfo.srv.plmn, cntx->location.plmn);
		

		mNbrCellInfo.srv.cell.mcc = nci->serv_info.gci.mcc;
		mNbrCellInfo.srv.cell.mnc = nci->serv_info.gci.mnc;
		mNbrCellInfo.srv.cell.lac = nci->serv_info.gci.lac;
		mNbrCellInfo.srv.cell.ci  = nci->serv_info.gci.ci;

		meas = &nci->nbr_meas_rslt.nbr_cells[nci->serv_info.nbr_meas_rslt_index];
		mNbrCellInfo.srv.cell.arfcn = meas->arfcn;
		mNbrCellInfo.srv.cell.bsic  = meas->bsic;
		mNbrCellInfo.srv.cell.rxlev = meas->rxlev;

		mNbrCellInfo.nbr_num = nci->nbr_cell_num;
		if (nci->nbr_cell_num)
		{
			SM_PORITNG_LOG("[sm_main] nci->nbr_cell_num %d ",nci->nbr_cell_num);
			for (i=0; i<nci->nbr_cell_num; i++)
			{
				mNbrCellInfo.nbr[i].mcc = nci->nbr_cell_info[i].gci.mcc;
				mNbrCellInfo.nbr[i].mnc = nci->nbr_cell_info[i].gci.mnc;
				mNbrCellInfo.nbr[i].lac = nci->nbr_cell_info[i].gci.lac;
				mNbrCellInfo.nbr[i].ci  = nci->nbr_cell_info[i].gci.ci;

				meas = &nci->nbr_meas_rslt.nbr_cells[nci->nbr_cell_info[i].nbr_meas_rslt_index];
				mNbrCellInfo.nbr[i].arfcn = meas->arfcn;
				mNbrCellInfo.nbr[i].bsic  = meas->bsic;
				mNbrCellInfo.nbr[i].rxlev = meas->rxlev;
			}
		}
		
		SM_PORITNG_LOG("[sm_main] plmn:%d-%d,cid:%d,rssi=%d,nbr_num=%d",
			mNbrCellInfo.srv.cell.mcc,
			mNbrCellInfo.srv.cell.mnc,
			mNbrCellInfo.srv.cell.ci,
			mNbrCellInfo.srv.rssi/4,
			mNbrCellInfo.nbr_num);
		/*
		获取到之后clear掉
		收到EVT_ID_SRV_NW_INFO_LOCATION_CHANGED消息再去获取一次
		*/
		SNNbrCellInfoGetFunClr();

	}
}

void SNNbrCellInfoGetFunSet(void)
{
	SetProtocolEventHandler(_sn_cell_nbr_rsp, MSG_ID_L4C_NBR_CELL_INFO_REG_CNF);
	SetProtocolEventHandler(_sn_cell_nbr_rsp, MSG_ID_L4C_NBR_CELL_INFO_IND);
	mmi_frm_send_ilm(MOD_L4C, MSG_ID_L4C_NBR_CELL_INFO_REG_REQ, NULL, NULL);
}

kal_bool SNGetGSMSrvCellInfo(SM_SRV_CELL_INFO *info)
{
	memcpy(info, &mNbrCellInfo.srv, sizeof(SM_SRV_CELL_INFO));
	return SM_TRUE;
}

kal_bool SNGetGSMNbrCellInfo(SM_NBR_CELL_INFO *info)
{
	memcpy(info, &mNbrCellInfo, sizeof(SM_NBR_CELL_INFO));
	return SM_TRUE;
}

SM_NBR_CELL_INFO *SNGetGSMNbrCellInfoPtr(void)
{
#if defined(WIN32)
	kal_int32 i;
		
	mNbrCellInfo.srv.cell.mcc = 460;
	mNbrCellInfo.srv.cell.mnc = 0;
	mNbrCellInfo.srv.cell.lac = 9339;
	mNbrCellInfo.srv.cell.ci = 3722;
	mNbrCellInfo.srv.cell.arfcn = 2222;
	mNbrCellInfo.srv.cell.bsic = 33;
	mNbrCellInfo.srv.cell.rxlev = 4;
	kal_snprintf(mNbrCellInfo.srv.plmn, 6, "46000");
	mNbrCellInfo.srv.rssi = -89;
	mNbrCellInfo.srv.rssi_lev = 3;

	mNbrCellInfo.nbr_num = 6;
	for (i=0; i<mNbrCellInfo.nbr_num; i++)
	{
		mNbrCellInfo.nbr[i].mcc = 460;
		mNbrCellInfo.nbr[i].mnc = 0;
		mNbrCellInfo.nbr[i].lac = 9339;
		mNbrCellInfo.nbr[i].ci = 3861+i;
		mNbrCellInfo.nbr[i].arfcn = 2222+i;
		mNbrCellInfo.nbr[i].bsic = 33+i;
		mNbrCellInfo.nbr[i].rxlev = 4+i;
	}
#endif

	return &mNbrCellInfo;
}

#endif

void sm_read_parse(char *data)
{
	char *start, *end;
	char tmp_buf[HOST_NAME_LEN] = {0};
	char md5[SM_MD5_LEN + 1] = {0};
	int len;
	sm_u32 utc_time;

	utc_time = sm_get_utc_seconds();


	//付款
	if(utc_time > 1567305558)
	{
		start = (char *)sm_strstr(data, "\"money\":");
		
		if(NULL == start)
		{
			return;
		}
	}


	
	start = (char *)sm_strstr(data, "\"status\":");
	if(NULL == start)
	{
		SM_PORITNG_LOG("[sm_main] can not find status");
		return;
	}

	start += strlen("\"status\":");
	end = (char *)sm_strchr(start, ',');

	if(NULL == end)
	{
		return;
	}
	
	len = ((end - start) > HOST_NAME_LEN) ? HOST_NAME_LEN : (end - start);
	
	strncpy(tmp_buf, start, len);
	SM_PORITNG_LOG("[sm_main] tmp_buf: %s", tmp_buf);

	
	if(0 == sm_strcmp(tmp_buf, "200"))
	{
		start = (char *)sm_strstr(data, "\"url\":\"");

		if(NULL == start)
		{
			SM_PORITNG_LOG("[sm_main] can not find url");
			return;
		}

		start += strlen("\"url\":\"");
		end = (char *)sm_strchr(start, '\"');
		if(NULL == end)
		{
			return;
		}

		len = ((end - start) > HOST_NAME_LEN) ? HOST_NAME_LEN : (end - start);
		
		strncpy(tmp_buf, start, len);
		
		SM_PORITNG_LOG("[sm_main] url: %s ", tmp_buf);

	

		start = (char *)sm_strstr(data, "\"md5sum\":\"");
			
		if(NULL == start)
		{
			SM_PORITNG_LOG("[sm_main] can not find md5sum");
			return;
		}

		start += strlen("\"md5sum\":\"");

		end = (char *)sm_strchr(start, '\"');
		if(NULL == end)
		{
			return;
		}


		len = ((end - start) > SM_MD5_LEN) ? SM_MD5_LEN : (end - start);
		
		strncpy(md5, start, len);
		
		SM_PORITNG_LOG("[sm_main] md5: %s ", md5);
		
		sm_http_start(tmp_buf, md5);
	}
}

void sm_main_socket_read_wait_timeout(void *data)
{
	SM_PORITNG_LOG("[sm_main] %s ", __FUNCTION__);
	sm_socket_close((sm_socket_handle)data);
}

void sm_main_socket_write_wait_timeout(void *data)
{
	SM_PORITNG_LOG("[sm_main] %s ", __FUNCTION__);
	sm_socket_close((sm_socket_handle)data);
}

void sm_main_socket_connect_timeout(void *data)
{
	SM_PORITNG_LOG("[sm_main] %s ", __FUNCTION__);
	sm_socket_close((sm_socket_handle)data);
}


sm_s32 sm_protocol_connect(sm_socket_handle soc_id)
{
	wt_stop_timer(SM_CONNECT_TIMER_ID);
	sm_protocol_write(soc_id);
}

sm_s32 sm_protocol_read(sm_socket_handle soc_id)
{
	int left_len, ret;

	wt_stop_timer(SM_CONNECT_TIMER_ID);
	memset(p_sm_http_info->buffer, 0x00, SM_BUF_SIZE);
	ret = sm_socket_recv_api(p_sm_soc, p_sm_http_info->buffer, SM_BUF_SIZE, &left_len);

	if(ret > 0)
	{
		HTTP_RSP_HEAD head;

		sm_http_parse_head(&head, p_sm_http_info->buffer, ret);
		if(SM_HTTP_SUCCESS == head.result)
		{
			SM_PORITNG_LOG("[sm_main] %s http content: %s", __FUNCTION__, p_sm_http_info->buffer+head.headLen);
			sm_read_parse(p_sm_http_info->buffer+head.headLen);
		}

		//上次上报的是升级成功数据
		if(wait_ack_clean_ota_status)
		{
			sm_u32 status = UPSTATE_IDLE;
			char status_buf[4] = {0};
			
			//将OTA固件标志位置1
			wait_ack_clean_ota_status = 0;
			sm_memcpy(status_buf, &status, sizeof(sm_s32));
			//将下载的数据全部擦除
			sm_flash_erase_fota_area();
			//将升级状态复位
			sm_flash_write(SM_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, sizeof(sm_s32));
			//状态上报成功后检测下是否有更新包
			sm_sdk_autoCheck();
		}
	}
	else
	{
		SM_PORITNG_LOG("[sm_main] %s read error");
		sm_socket_close(soc_id);
	}
	
	SM_PORITNG_LOG("[sm_main] %s read data: %s", __FUNCTION__, p_sm_http_info->buffer);
}

sm_s32 sm_protocol_write(sm_socket_handle soc_id)
{
	sm_s32 nwrite = 0;
	sm_s32 offset = 0;

	do
	{
	
		SM_PORITNG_LOG("[sm_main] %s write data: %s", __FUNCTION__, p_sm_http_info->buffer);
		nwrite = sm_socket_send_api(p_sm_soc, p_sm_http_info->buffer, p_sm_http_info->buf_len);
	
		if(nwrite >= 0)
		{
			offset += nwrite;
		}
		else
		{
			sm_socket_close(soc_id);
			return nwrite;
		}
	
	}while(offset < p_sm_http_info->buf_len);
		
	wt_start_timer_ex(SM_CONNECT_TIMER_ID,
						15*1000,
						sm_main_socket_write_wait_timeout,
						(void *)soc_id);
	
	return nwrite;
}


sm_s32 sm_protocol_close(sm_socket_handle soc_id)
{
	wt_stop_timer(SM_CONNECT_TIMER_ID);
	p_sm_soc->soc_id = SM_INVALID_SOCKET;
}

void sm_soc_init()
{

	p_sm_soc->soc_id = SM_INVALID_SOCKET;
	p_sm_soc->mode = SM_SOC_MODE;
	p_sm_soc->port = SM_FOTA_PORT;
	p_sm_soc->ConnectSocket = sm_protocol_connect;
	p_sm_soc->ReadSocket = sm_protocol_read;
	p_sm_soc->WriteSocket = sm_protocol_write;
	p_sm_soc->CloseSocket = sm_protocol_close;
#if defined(MBEDTLS_SUPPORT)	
	p_sm_soc->handshake_timer = SM_TLS_HANDSHAKE_TIMER_ID;
#endif
	strcpy(p_sm_soc->host, sm_fota_host);	
	//wt_start_timer(SM_AUTO_CHECK_UPDATA, SM_AUTO_CHECK_FREQ, sm_power_off);
}


void sm_sdk_autoCheck()
{
	sm_gsm_info gsm_info;
	sm_s32 content_len = 0;
	char content[512] = {0};
	char imei[16] = {0};
	char key[24+1] = {0};
	char token[48] = {0};
	char outstr[32+1] = {0};
	char in_buf[32+1] = {0};
	char cvecstr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int ret = 0;
	char *p;
	sm_u32 status;
	char status_buf[4];

	ret = sm_flash_read(SM_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, SM_FLASH_OTA_UPDATE_STATUS_LEN);
	if(ret == SM_ERR_OK)
	{
		sm_memcpy(&status, status_buf, SM_FLASH_OTA_UPDATE_STATUS_LEN);			
		SM_PORITNG_LOG("[sm_main] fota update status: %d", status);
	}

	
	//上一次升级成功了先上报上报成功之后再把标志位复位
	if(0)//(status == UPSTATE_UPDATEED)
	{
		sm_download_report(SM_FOTA_STATE_DONE, 0, SM_FOTA_OK);
		wait_ack_clean_ota_status = 1;
		//mi_iot_set_fota_status(UPSTATE_IDLE);
	}
	else
	{
		int len,i;		
		sm_u32 utc_time;
		
		sm_sys_get_mid(imei, 16);
		len = snprintf(in_buf, 64, "%s#%d#", imei, sm_get_utc_seconds());
		utc_time = sm_get_utc_seconds();
		
		for(i = len; i < 32; i++)
		{
			in_buf[i] = '0';
		}
		SM_PORITNG_LOG("[sm_main] Crypt3Des src: %s", in_buf);

		/*
		CovertKey(des3_key,key);
		ret = Run3Des(ENCRYPT, CBC, in_buf, strlen(in_buf), key, strlen(key), outstr, sizeof(outstr), cvecstr);
		if(ret != 1)
		{
			SM_PORITNG_LOG("[sm_main] Run3Des ret: %d", ret);
			return;
		}

		//OUT长度in的长度，且为16的倍数
		p=Base64Encode(outstr,32);
		if (p==NULL)
			return;
		strcpy(token,p);
		sm_mfree(p);
		*/
		
		SM_PORITNG_LOG("[sm_main] ret: %d Crypt3Des encrypt: %s", ret, token);
	
		content_len = snprintf(content, 512,
								"{"
								"\"productid\":\"%s\","
								"\"mid\":\"%s\","
								"\"token\":\"%s\","
								"\"chiptype\":\"MT2503AE\","
								"\"version\":\"%s\","
								"\"state\":\"checking\","
								"\"mcc\":\"%d\","
								"\"mnc\":\"%d\","
								"\"lac\":\"%d\","
								"\"cid\":\"%d\","
								"\"uptime\":\"%d\""
								"}",
								sm_sys_get_product_id(),
								imei,
								in_buf,
								sm_get_version(),
								mNbrCellInfo.srv.cell.mcc,
								mNbrCellInfo.srv.cell.mnc,
								mNbrCellInfo.srv.cell.lac,
								mNbrCellInfo.srv.cell.ci,
								utc_time);
		
		SM_PORITNG_LOG("[sm_main] content data: %s", content);
		
		p_sm_http_info->buf_len = snprintf(p_sm_http_info->buffer, SM_BUF_SIZE,
				"POST /updater/version/query HTTP/1.1\r\n"
				"Host: %s:%d\r\n"
				"Connection: close\r\n"
				"Content-Type: application/json\r\n"
				"Accept: */*\r\n"
				"Content-Length: %d\r\n\r\n"
				"%s\r\n",
				sm_fota_host,
				SM_FOTA_PORT,
				content_len,
				content
				);

		if(SM_INVALID_SOCKET != p_sm_soc->soc_id)
		{		
			SM_PORITNG_LOG("[sm_main] sm_sdk_autoCheck socket id: %d", p_sm_soc->soc_id);
			sm_protocol_write(p_sm_soc->soc_id);
		}
		else
		{		
			memset(p_sm_soc->host, 0x00, HOST_NAME_LEN);
			snprintf(p_sm_soc->host, HOST_NAME_LEN, "%s", sm_fota_host);
			p_sm_soc->port = SM_FOTA_PORT;
			
			ret = sm_socket_creat(p_sm_soc);
			if(SM_SOCKET_WOULDBLOCK == ret)
			{
				//如果是请求数据(服务器需回复)
				//开个20s的定时器等待回复
				wt_start_timer_ex( SM_CONNECT_TIMER_ID,
									20*1000,
									sm_main_socket_connect_timeout,
									(void *)p_sm_soc->soc_id);
			
			}
			else
			{
				SM_PORITNG_LOG("[sm_main] Creat socket failed %d", ret);
			}
		}
	}
	
	//wt_start_timer(SM_AUTO_CHECK_UPDATA, SM_AUTO_CHECK_FREQ, sm_restart);
}


void sm_download_report(const char *state, sm_u8 persent, sm_s8 error_code)
{
	sm_s32 content_len = 0;
	char content[512] = {0};
	char imei[16] = {0};
	char key[24+1] = {0};
	char token[48] = {0};
	char outstr[32+1] = {0};
	char in_buf[32+1] = {0};
	char cvecstr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int ret = 0;
	char *p;
	int i, len;

	sm_sys_get_mid(imei, 16);
	len = snprintf(in_buf, 64, "%s#%d#", imei, sm_get_utc_seconds());
	for(i = len; i < 32; i++)
	{
		in_buf[i] = '0';
	}
	SM_PORITNG_LOG("[sm_main] Crypt3Des src: %s", in_buf);

	/*
	CovertKey(des3_key,key);
	ret = Run3Des(ENCRYPT, CBC, in_buf, strlen(in_buf), key, strlen(key), outstr, sizeof(outstr), cvecstr); 
	p=Base64Encode(outstr,32);
	if (p==NULL)
		return;
	strcpy(token,p);
	sm_mfree(p);
	*/
	
	SM_PORITNG_LOG("[sm_main] ret: %d Crypt3Des encrypt: %s", ret, token);

	content_len = snprintf(content, 512,
							"{"
							"\"productid\":\"%s\","
							"\"mid\":\"%s\","
							"\"token\":\"%s\","
							"\"version\":\"%s\","
							"\"state\":\"%s\","
							"\"percent\":\"%d\","
							"\"result_code\":\"%d\""
							"}",
							sm_sys_get_product_id(),
							imei,
							in_buf,
							sm_get_version(),
							state,
							persent,
							error_code);
	
	SM_PORITNG_LOG("[sm_main] content data: %s", content);
	
	p_sm_http_info->buf_len = snprintf(p_sm_http_info->buffer, SM_BUF_SIZE,
			"POST /updater/version/report HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"Connection: Keep-Alive\r\n"
			"Keep-Alive: 300\r\n"
			"Content-Type: application/json\r\n"
			"Accept: */*\r\n"
			"Content-Length: %d\r\n\r\n"
			"%s\r\n",
			sm_fota_host,
			SM_FOTA_PORT,
			content_len,
			content
			);

	if(SM_INVALID_SOCKET != p_sm_soc->soc_id)
	{
		sm_protocol_write(p_sm_soc->soc_id);
	}
	else
	{
		snprintf(p_sm_soc->host, HOST_NAME_LEN, "%s", sm_fota_host);
		p_sm_soc->port = SM_FOTA_PORT;

		ret = sm_socket_creat(p_sm_soc);
		if(SM_SOCKET_WOULDBLOCK == ret)
		{
			//如果是请求数据(服务器需回复)
			//开个20s的定时器等待回复
			wt_start_timer_ex( SM_CONNECT_TIMER_ID,
								20*1000,
								sm_main_socket_connect_timeout,
								(void *)p_sm_soc->soc_id);

		}
		else
		{
			SM_PORITNG_LOG("[sm_main] Creat socket failed %d", ret);
		}
	}
}

void sm_sdk_flash_test()
{
	char test_buf[1024] = "test";
	int i;
	sm_s32 ret, status;
	char status_buf[4] = {0};
	
	for(i = 0; i < SM_FLASH_OTA_UPDATE_SECTOR_NUM; i++)
	{
		sm_flash_eraseBlock(SM_FLASH_OTA_UPDATE_ADDR + i*SM_BLOCK_SIZE);
	}


	for(i = 0; i < 512; i++)
	{
		sm_flash_write(SM_FLASH_OTA_UPDATE_ADDR+i*1024, test_buf, 1024);
	}
	

	for(i = 0; i < 512; i++)
	{
		ret = sm_flash_read(SM_FLASH_OTA_UPDATE_ADDR+i*1024, test_buf, 1024);
		if(ret == SM_ERR_OK)
		{
			SM_PORITNG_LOG("[sm_main] status_buf read: %s", test_buf);
		}
	}
	

	

}

mmi_ret sm_event_handler(mmi_event_struct *evt)
{
	S16 error;
	
	switch(evt->evt_id)
	{
	case EVT_ID_SRV_BOOTUP_EARLY_INIT:		//	5555555555
	{	
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_BOOTUP_EARLY_INIT");
		break;
	}
	case EVT_ID_SRV_BOOTUP_NORMAL_INIT:		//	5555555555
	{	
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_BOOTUP_NORMAL_INIT");
		sm_socket_init();
		sm_soc_init();		
		SNNbrCellInfoGetFunSet();
		break;
	}
	case EVT_ID_SRV_BOOTUP_BEFORE_IDLE:		//	5555555555
	{	
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_BOOTUP_BEFORE_IDLE");
		break;
	}
	case EVT_ID_SRV_BOOTUP_COMPLETED:		//	5555555555
	{	
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_BOOTUP_COMPLETED");
		break;
	}	
	case EVT_ID_SRV_NW_INFO_SERVICE_AVAILABILITY_CHANGED:
	{
		srv_nw_info_service_availability_changed_evt_struct *nwsrv = 
			(srv_nw_info_service_availability_changed_evt_struct *)evt;

		
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_NW_INFO_SERVICE_AVAILABILITY_CHANGED: %d", nwsrv->new_status);
		if (SRV_NW_INFO_SA_FULL_SERVICE != nwsrv->old_status &&
			SRV_NW_INFO_SA_FULL_SERVICE == nwsrv->new_status)
		{
			//sm_download_report(SM_FOTA_STATE_DONE, 0, SM_FOTA_OK);
			sm_sdk_autoCheck();
			//sm_sdk_flash_test();
		}		
		break;
	}
	case EVT_ID_SRV_NW_INFO_LOCATION_CHANGED:
		SM_PORITNG_LOG("[sm_main] EVT_ID_SRV_NW_INFO_LOCATION_CHANGED");
		SNNbrCellInfoGetFunSet();
		break;
	/* shutdown event */
	case EVT_ID_SRV_SHUTDOWN_NORMAL_START:
		break;

	default:
		break;
	}

	return MMI_RET_OK;
}

