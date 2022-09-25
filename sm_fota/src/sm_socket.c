#include "sm_socket.h"
#include "sm_debug.h"
#include "sm_main.h"


//MTK文件
#include "NwInfoSrvGprot.h"
#include "cbm_api.h"
#include "soc_api.h"
#include "DtcntSrvIprot.h"
#include "DtcntSrvGprot.h"
#include "soc_consts.h"



//MMI_APPLICATION_T g_sm_app = {sm_dispatch_soc_event, CT_APPLICATION, SM_NULL};

sm_socket_list_struct *SOC_NodeList = SM_NULL;		//TCP首节点
sm_u8 g_smfota_nsapi = 0;
sm_u32 fota_data_account = 0;
sm_u8 fota_app_id = 0;


const char *sm_pers = "sm_client";

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
const unsigned char sm_psk[] = "23wy1nsHD0qZKHPg";
const char sm_psk_id[] = "DID=11671894";
static char sm_psk_key[32];
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
/* This is tests/data_files/test-ca2.crt, a CA using EC secp384r1 */
const unsigned char sm_ca_cert[] = { "\
-----BEGIN CERTIFICATE-----\n\
MIIBazCCAQ+gAwIBAgIEA/UKYDAMBggqhkjOPQQDAgUAMCIxEzARBgNVBAoTCk1p\n\
amlhIFJvb3QxCzAJBgNVBAYTAkNOMCAXDTE2MTEyMzAxMzk0NVoYDzIwNjYxMTEx\n\
MDEzOTQ1WjAiMRMwEQYDVQQKEwpNaWppYSBSb290MQswCQYDVQQGEwJDTjBZMBMG\n\
ByqGSM49AgEGCCqGSM49AwEHA0IABL71iwLa4//4VBqgRI+6xE23xpovqPCxtv96\n\
2VHbZij61/Ag6jmi7oZ/3Xg/3C+whglcwoUEE6KALGJ9vccV9PmjLzAtMAwGA1Ud\n\
EwQFMAMBAf8wHQYDVR0OBBYEFJa3onw5sblmM6n40QmyAGDI5sURMAwGCCqGSM49\n\
BAMCBQADSAAwRQIgchciK9h6tZmfrP8Ka6KziQ4Lv3hKfrHtAZXMHPda4IYCIQCG\n\
az93ggFcbrG9u2wixjx1HKW4DUA5NXZG0wWQTpJTbQ==\n\
-----END CERTIFICATE-----\n\
" };
#endif /* MBEDTLS_X509_CRT_PARSE_C */

void sm_socket_set_netid(sm_u8 netid)
{
	g_smfota_nsapi = netid;
}

sm_u8 sm_socket_get_netid()
{
	return g_smfota_nsapi;
}


sm_u32 sm_encode_dataaccount_id(void)
{
	sm_u32 dtacc;
	srv_dtcnt_prof_str_info_qry_struct apn;
	
	if(fota_data_account == 0)
	{
        cbm_app_info_struct info = {0};
        sm_s8 result = 0;


        if(0 == fota_app_id)
        {
            memset(&info, 0, sizeof(info));            
            info.app_str_id = 0;
            info.app_icon_id = 0;
			info.app_type = DTCNT_APPTYPE_DEF |
				DTCNT_APPTYPE_BRW_WAP |
				DTCNT_APPTYPE_BRW_HTTP |
				DTCNT_APPTYPE_EMAIL |
				DTCNT_APPTYPE_WIDGET |
				DTCNT_APPTYPE_PUSH |
				DTCNT_APPTYPE_PLAYER |
				DTCNT_APPTYPE_MRE_NET | 
				DTCNT_APPTYPE_TETHERING |
				DTCNT_APPTYPE_NTP;
		 
            result = cbm_register_app_id_with_app_info(&info, &fota_app_id);
            SM_PORITNG_LOG("cbm_register_app_id_with_app_info, result = [%d]\n\r", result);
        }
		
		#if 0	
		// encode account
		dtacc = srv_dtcnt_sn_get_acct();
		if(-1 == dtacc)
		{
			fota_data_account = 0;
		}
		else
		{
			fota_data_account = cbm_encode_data_account_id(
				dtacc,
				CBM_SIM_ID_SIM1, 
				fota_app_id,
				0);
		}
		#endif

		fota_data_account = cbm_encode_data_account_id(CBM_DEFAULT_ACCT_ID, 0, fota_app_id, 0);

	}
	
	SM_PORITNG_LOG("fota_data_account = [%d]\n\r", fota_data_account);

	return fota_data_account;
}


sm_s32 sm_socket_extract_ip_addr(sm_s8* szDstBuf, const sm_s8* szSrcURL, sm_u32 *nHostPort )
{
	int ret = 0;
	char portBuf[10];
	char* portPtr;

	// check "http" 
	if ( sm_memcmp(szSrcURL, "http://", 7) == 0 )
	{
		szSrcURL += 7;
	}
	else if ( sm_memcmp(szSrcURL, "https://", 8) == 0 )
	{
		szSrcURL += 8;
		ret = 1;
	}

	// copy data 
	while ( *szSrcURL != 0 && *szSrcURL != ':' && *szSrcURL != '/' )
	{
		*szDstBuf++ = *szSrcURL++;
	}
	*szDstBuf = 0;

	if (*szSrcURL == ':')
	{
		long port = 0;
		portPtr = &portBuf[0];
		szSrcURL++;
		*portPtr++ = *szSrcURL++;
		while ( *szSrcURL != 0 && *szSrcURL != '/' )
		{
			*portPtr++ = *szSrcURL++;
		}
		*portPtr = 0;
		port = sm_atoi(portBuf);
		*nHostPort = port;
	}
	else
	{
		*nHostPort = 80;
	}

	return ret;
}

sm_socket_list_struct *sm_socket_get_node(sm_socket_handle soc_id)
{
	sm_socket_list_struct *node = SOC_NodeList;

	while(node && node->soc_id != soc_id)
	{
		node = node->next;
	}

	return node;
}

sm_socket_list_struct *sm_socket_get_node_by_dns_id(sm_s32 dns_id)
{
	sm_socket_list_struct *node = SOC_NodeList;

	while(node && node->connect.dns_id != dns_id)
	{
		node = node->next;
	}

	return node;
}

sm_bool sm_socket_delete_node(sm_socket_list_struct *node)
{
	sm_socket_list_struct *next;
	sm_socket_list_struct *head;

	if(node == SM_NULL)
		return;

	head = node->head;			//当前节点的上一个
	next = node->next;			//当前节点的下一个

	if(head == SM_NULL)
	{
		//在首
		SOC_NodeList = next;
		if(SOC_NodeList != SM_NULL)
			SOC_NodeList->head = SM_NULL;		//此时在首
	}
	else
	{
		head->next = next;
	}

	return (SOC_NodeList == SM_NULL) ? TRUE: FALSE;
}

void sm_socket_insert_node(sm_socket_list_struct *node)
{
	if(SOC_NodeList == SM_NULL)
	{//第一个
		SOC_NodeList = node;
		SOC_NodeList->head = SM_NULL;
		SOC_NodeList->next = SM_NULL;
	}
	else
	{
		//增加的节点放在首位
		SOC_NodeList->head = (void *)node;
		node ->next = (void *)SOC_NodeList;
		node ->head = SM_NULL;
		SOC_NodeList = node;
	}
}

void sm_socket_connect(sm_socket_handle soc_id)
{
	sm_s32 result = 0;
	sm_socket_list_struct *node = sm_socket_get_node(soc_id);

	if(node != SM_NULL)
	{
		if(SM_TLS_MODE == node->mode)
		{
		#if defined(MBEDTLS_SUPPORT)
			result = sm_tls_handshake(node);
			if(SM_SOCKET_SUCCESS == result)
			{
				if(node->ConnectSocket)
					node->ConnectSocket(soc_id);

			}
			else if(SM_SOCKET_WOULDBLOCK == result)
			{
				//握手阻塞了,等待...
				wt_start_timer_ex(	node->handshake_timer,
									5000,
									sm_tls_handshake_timeout,
									(void *)node->soc_id);

			}
			else
			{
				SM_PORITNG_LOG("[sm_soc] Connect succes but tls handshake fail...");
				sm_socket_close(soc_id);
			}
		#endif
		}
		else
		{
			if(node->ConnectSocket)
				node->ConnectSocket(soc_id);
		}
	}
	else
	{
		SM_PORITNG_LOG("[sm_soc] socket connect, but socket not match\n\r");
	}

}

void sm_socket_close(sm_socket_handle soc_id)
{
	sm_socket_list_struct *node = sm_socket_get_node(soc_id);

	if(node != SM_NULL)
	{
	#if defined(MBEDTLS_SUPPORT)
		sm_tls_free(&node->tls);
	#endif	
		soc_shutdown((sm_s8)node->soc_id, SHUT_RDWR);
		soc_close((sm_s8)node->soc_id);
		SM_PORITNG_LOG("[sm_soc] SOC_CLOSE..%x\r\n", node->CloseSocket);
		if(node->CloseSocket)
			node->CloseSocket(soc_id);

		sm_socket_delete_node(node);

	}
	else
	{
		SM_PORITNG_LOG("[sm_soc] socket close, but socket not match\n\r");
	}

}

void sm_socket_read(sm_socket_handle soc_id)
{
	sm_socket_list_struct *node = sm_socket_get_node(soc_id);

	if(node != SM_NULL)
	{
	#if defined(MBEDTLS_SUPPORT)
		if(node->tls && node->tls->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			int ret;

			SM_PORITNG_LOG("[sm_soc] %s handshake state: %d\r\n", __FUNCTION__, node->tls->ssl.state);
			ret = mbedtls_ssl_handshake(&node->tls->ssl);

			if(((ret = mbedtls_ssl_handshake(&node->tls->ssl)) != 0))
			{
        		if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        		{
					wt_start_timer_ex(node->handshake_timer,
										5000,
										sm_tls_handshake_timeout,
										(void *)node->soc_id);

				}
				else
				{
					SM_PORITNG_LOG("[sm_soc] handshake failed %d...", ret);
					sm_stop_timer(node->handshake_timer);
					sm_socket_close(soc_id);
				}
			}
			else
			{
				//握手成功之后关闭TLS握手定时器,调用写接口
				sm_stop_timer(node->handshake_timer);
				if(node->WriteSocket)
					node->WriteSocket(soc_id);
			}

		}
		else
	#endif
		{
			if(node->ReadSocket)
				node->ReadSocket(soc_id);
		}
	}
	else
	{
		SM_PORITNG_LOG("[sm_soc] socket read, but socket not match\n\r");
	}
}

void sm_socket_write(sm_socket_handle soc_id)
{
	sm_socket_list_struct *node = sm_socket_get_node(soc_id);

	if(node != SM_NULL)
	{
	#if defined(MBEDTLS_SUPPORT)
		if(node->tls && node->tls->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			int ret;

			SM_PORITNG_LOG("[sm_soc] %s handshake state: %d\r\n", __FUNCTION__, node->tls->ssl.state);
			ret = mbedtls_ssl_handshake(&node->tls->ssl);

			if(((ret = mbedtls_ssl_handshake(&node->tls->ssl)) != 0))
			{
        		if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        		{
					wt_start_timer_ex(node->handshake_timer,
										3000,
										sm_tls_handshake_timeout,
										(void *)node->soc_id);

				}
				else
				{
					SM_PORITNG_LOG("[sm_soc] handshake failed %d...", ret);
					sm_stop_timer(node->handshake_timer);
					sm_socket_close(soc_id);
				}
			}
			else
			{
				//握手成功之后关闭TLS握手定时器,调用写接口
				sm_stop_timer(node->handshake_timer);
				if(node->WriteSocket)
					node->WriteSocket(soc_id);
			}

		}
		else
	#endif
		{
			SM_PORITNG_LOG("[sm_soc] SOC_READ..%x\r\n", node->WriteSocket);
			if(node->WriteSocket)
				node->WriteSocket(soc_id);
		}
	}
	else
	{
		SM_PORITNG_LOG("[sm_soc] socket write, but socket not match\n\r");
	}
}

sm_s32 sm_socket_creat(sm_socket_list_struct *node)
{
	//在这里创建socket,将事件的回调函数加入列表
	sm_u32 acctId;
	sm_bool host_is_ip = KAL_FALSE;
	sm_s8 soc_ret;	
	sm_u8 addr_len, option;	
	sockaddr_struct addr;

	if(SM_NULL == node)
	{
		SM_PORITNG_LOG("[sm_soc] Creat socket fail, node is SM_NULL...");
		return SM_INVALID_SOCKET;
	}

	acctId = sm_encode_dataaccount_id();
	
	node->soc_id = soc_create(SOC_PF_INET, SOC_SOCK_STREAM, 0, MOD_MMI, acctId);
	if(node->soc_id < 0)
	{
		SM_PORITNG_LOG("[sm_soc] create socket fail id = %d\n", node->soc_id);		
		if(node->CloseSocket)
			node->CloseSocket(node->soc_id);
		return SM_INVALID_SOCKET;
	}


	option = SM_TRUE;
	soc_ret = soc_setsockopt((sm_s8)node->soc_id, SOC_NBIO, &option, sizeof(option));
	if (SOC_SUCCESS != soc_ret)
	{
		SM_PORITNG_LOG("[sm_soc] soc_setsockopt error = %d\n", soc_ret);
		soc_close(node->soc_id);
		if(node->CloseSocket)
			node->CloseSocket(node->soc_id);
		return SM_INVALID_SOCKET;
	}
	
	option = SOC_READ|SOC_WRITE|SOC_CONNECT|SOC_CLOSE;
	soc_ret = soc_setsockopt((sm_s8)node->soc_id, SOC_ASYNC, &option, sizeof(option));
	if (SOC_SUCCESS != soc_ret)
	{	
		SM_PORITNG_LOG("[sm_soc] soc_setsockopt error = %d\n", soc_ret);
		soc_close(node->soc_id);
		if(node->CloseSocket)
			node->CloseSocket(node->soc_id);
		return SM_INVALID_SOCKET;
	}


	soc_ip_check(node->host, addr.addr, &host_is_ip);
	if(host_is_ip)
	{//主机是IP地址
		addr.port = node->port;
		addr.sock_type = SOC_SOCK_STREAM;
		addr.addr_len = 4;
		soc_ret = soc_connect((sm_s8)node->soc_id, &addr);
		SM_PORITNG_LOG("[SOCKET] Host Is IP %d.%d.%d.%d:%d\r\n",
			addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3], addr.port);
		switch (soc_ret)
		{
			case SOC_SUCCESS:
			case SOC_WOULDBLOCK:
				break;
			default:
				SM_PORITNG_LOG("[sm_soc] soc_connect error = %d\n", soc_ret);
				soc_close(node->soc_id);
				if(node->CloseSocket)
					node->CloseSocket(node->soc_id);
				return SM_INVALID_SOCKET;
				break;
		}
	}
	else
	{
		//主机是域名
		soc_ret = soc_gethostbyname(KAL_FALSE, MOD_MMI, (sm_s8)node->soc_id, node->host, addr.addr, &addr_len, 0, acctId);
		switch (soc_ret)
		{
			case SOC_SUCCESS:
			case SOC_WOULDBLOCK:
				break;
			default:			
				SM_PORITNG_LOG("[sm_soc] soc_gethostbyname error = %d\n", soc_ret);
				soc_close(node->soc_id);
				if(node->CloseSocket)
					node->CloseSocket(node->soc_id);
				return SM_INVALID_SOCKET;
				break;
		}
		
	}
	
	sm_socket_insert_node(node);
	return SM_SOCKET_WOULDBLOCK;
}

/*读接口*/
sm_s32 sm_socket_recv_api(sm_socket_list_struct *node, char * buf, int len, int *left_len)
{
	sm_s32 ret;
	sm_s32 sock_error_code;
	
#if defined(MBEDTLS_SUPPORT)
	if(NULL == node)
		return -1;

	if(SM_TLS_MODE == node->mode)
		ret = mbedtls_ssl_read(&node->tls->ssl, buf, len);
	else
#endif
	ret = soc_recv(node->soc_id, buf, len, 0);
	if(SOC_WOULDBLOCK == ret)
		return 0;
	else
		return ret;
}

/*写接口*/
sm_s32 sm_socket_send_api(sm_socket_list_struct *node, char* buf, int len)
{
	sm_s32 ret;
	sm_s32 sock_error_code;
	
#if defined(MBEDTLS_SUPPORT)
	if(NULL == node)
		return -1;

	if(SM_TLS_MODE == node->mode)
		ret = mbedtls_ssl_write(&node->tls->ssl, buf, len);
	else
#endif
		ret = soc_send(node->soc_id, buf, len, 0);

	if(SOC_WOULDBLOCK == ret)
		return 0;
	else
		return ret;
}

sm_bool sm_gethostbyname_handle(void* inMsg)
{
	sm_s8 soc_ret;
	sm_bool result = SM_FALSE;
	sm_socket_list_struct *node;
    app_soc_get_host_by_name_ind_struct* dns_ind = (app_soc_get_host_by_name_ind_struct*)inMsg;

	if (dns_ind)
	{
		node = (sm_socket_list_struct *)sm_socket_get_node(dns_ind->request_id);
		if(NULL != node)
		{
			if(SM_TRUE == dns_ind->result)
			{
				sockaddr_struct addr;
				
				memset(&addr, 0x00, sizeof(addr));
				memcpy(addr.addr, dns_ind->addr, dns_ind->addr_len);
				addr.addr_len = dns_ind->addr_len;
				addr.port = node->port;
				SM_PORITNG_LOG("[sm_soc] Host Isn't IP %d.%d.%d.%d:%d\r\n",
					addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3], addr.port);
				
				soc_ret = soc_connect(node->soc_id, &addr);
				if(SOC_SUCCESS == soc_ret || SOC_WOULDBLOCK == soc_ret)
				{
					result = SM_TRUE;
				}
			}
			else
			{
				SM_PORITNG_LOG("[sm_soc] get hostbyname error: %d\r\n", dns_ind->result);
			}
		}
		else
		{
			SM_PORITNG_LOG("[sm_soc] other app use dns parse don't care\r\n");
		}
	}
	
	return result;
}

sm_bool sm_soc_message_dispatch(void *sig_ptr)
{
	S32 soc_ret;
    app_soc_notify_ind_struct *msg = (app_soc_notify_ind_struct*)sig_ptr;

	
	if(msg == NULL || NULL == sm_socket_get_node(msg->socket_id))
		return MMI_FALSE;

	SM_PORITNG_LOG("[sm_soc] sm_soc_message_dispatch: %d\r\n", msg->event_type);
	switch(msg->event_type)
	{
		case SOC_WRITE:
			sm_socket_write(msg->socket_id);
			break;
		case SOC_READ:
			sm_socket_read(msg->socket_id);
			break;
		case SOC_CONNECT:
			if(msg->result)
			{
				sm_socket_connect(msg->socket_id);
			}
			else
			{	
				SM_PORITNG_LOG("[sm_soc] connect error code: %d, %d\r\n", msg->error_cause, msg->detail_cause);
				sm_socket_close(msg->socket_id);
			}
			break;
		case SOC_CLOSE:			
			SM_PORITNG_LOG("[sm_soc] close cause: %d, %d\r\n", msg->error_cause, msg->detail_cause);
			sm_socket_close(msg->socket_id);
			break;
	}
	
	return SM_TRUE;
}

void sm_socket_init(void)
{
	//Register event handler
	mmi_frm_set_protocol_event_handler(MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND, (PsIntFuncPtr)sm_gethostbyname_handle, MMI_TRUE);//共享模式
	mmi_frm_set_protocol_event_handler(MSG_ID_APP_SOC_NOTIFY_IND, (PsIntFuncPtr)sm_soc_message_dispatch, MMI_TRUE);//共享模式
}

#if defined(MBEDTLS_SUPPORT)
/*
 * Read at most 'len' characters
 */
int sm_tls_recv( void *ctx, unsigned char *buf, size_t len )
{
    int ret;
    int fd = ((mbedtls_net_context *) ctx)->fd;
    int sock_error_code = 0;
	int left_len;


    if( fd < 0 )
        return( MBEDTLS_ERR_NET_INVALID_CONTEXT );

	ret = (int) soc_recv( fd, buf, len, NULL);

   	SM_PORITNG_LOG("[sm_soc] %s %d %d", __FUNCTION__, ret, left_len);
    if( ret < 0 )
    {
        sock_error_code = ret;

		SM_PORITNG_LOG("[sm_soc] %s error: %d", __FUNCTION__, sock_error_code);

		if(SOC_WOULDBLOCK == sock_error_code)
			return( MBEDTLS_ERR_SSL_WANT_READ );

        return( MBEDTLS_ERR_NET_RECV_FAILED );
    }

    return( ret );
}

/*
 * Write at most 'len' characters
 */
int sm_tls_send( void *ctx, const unsigned char *buf, size_t len )
{
    int ret;
    int fd = ((mbedtls_net_context *) ctx)->fd;
    int sock_error_code = 0;

    if( fd < 0 )
        return( MBEDTLS_ERR_NET_INVALID_CONTEXT );

    ret = (int) soc_send( fd, buf, len, 0);

   	SM_PORITNG_LOG("[sm_soc] %s %d", __FUNCTION__, ret);
    if( ret < 0 )
    {
        sock_error_code = ret;

   		SM_PORITNG_LOG("[sm_soc] %s error: %d", __FUNCTION__, sock_error_code);

		if(SOC_WOULDBLOCK == sock_error_code)
			return( MBEDTLS_ERR_SSL_WANT_WRITE );

        return( MBEDTLS_ERR_NET_SEND_FAILED );
    }

    return( ret );
}

void sm_mbedtls_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
	if(NULL != str)
	{
		SM_PORITNG_LOG("[MBEDTLS] %s", str);
	}
}

sm_s32 sm_tls_handshake_timeout(void *data)
{
	sm_socket_close((sm_socket_handle)data);
}

sm_s32 sm_tls_handshake(sm_socket_list_struct *node)
{
	//char key[SM_NV_PSK_KEY_LEN] = (0);
	//char id[SM_NV_PSK_ID_LEN + 4] = {0};
	mbedtls_net_context *server_fd;
	mbedtls_entropy_context *entropy;
	mbedtls_ctr_drbg_context *ctr_drbg;
	mbedtls_ssl_context *ssl;
	mbedtls_ssl_config *conf;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt *ca;
#endif

    int result;



	if(node == SM_NULL)
	{
		SM_PORITNG_LOG("[sm_soc] SM_BAD_INPUT");
		return SM_BAD_INPUT;
	}

	SM_PORITNG_LOG("[sm_soc] 1 node->tls: %x", node->tls);
    if(node->tls == NULL)
    {
		node->tls = (tls_connect_t *)sm_malloc(sizeof(tls_connect_t));

		if(node->tls == NULL)
		{
			SM_PORITNG_LOG("[sm_soc] SM_SOCKET_NO_MATCH");
			return SM_MALLOC_TLS_FAILED;
		}

	}
	
	SM_PORITNG_LOG("[sm_soc] 2 node->tls: %x", node->tls);
	
	server_fd = &node->tls->server_fd;
    entropy = &node->tls->entropy;
    ctr_drbg = &node->tls->ctr_drbg;
    ssl = &node->tls->ssl;
    conf = &node->tls->conf;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    ca = &node->tls->ca;
#endif

	/*
     * 1. Initialize and setup stuff
     */
    server_fd->fd = -1;
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_init(ca);
#endif

    mbedtls_entropy_init(entropy);

    if( mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                       (const unsigned char *)sm_pers, strlen(sm_pers)) != 0 )
    {
		return SM_CTR_DRBG_SEED_FAILED;
    }

    if( mbedtls_ssl_config_defaults(conf,
                MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT) != 0 )
    {
		return SM_SSL_CONFIG_DEFAULTS_FAILED;
    }

    //mbedtls_ssl_conf_min_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MAJOR_VERSION_3);
    //mbedtls_ssl_conf_max_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MAJOR_VERSION_3);

    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
	mbedtls_ssl_conf_dbg(conf, sm_mbedtls_debug, NULL);


#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	#if 1//defined(WIN32)
    mbedtls_ssl_conf_psk(conf, sm_psk, sizeof(sm_psk) - 1,
                (const unsigned char *) sm_psk_id, sizeof( sm_psk_id ) - 1 );
	#else
	sm_get_psk_id(id, SM_NV_PSK_ID_LEN + 4);
	sm_get_psk_key(key, SM_NV_PSK_KEY_LEN);
    mbedtls_ssl_conf_psk(conf, key, sizeof(key),
            (const unsigned char *)id, sizeof(id));
	#endif
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if( mbedtls_x509_crt_parse(ca, sm_ca_cert, sizeof( sm_ca_cert ) ) != 0 )
    {
		//没有证书直接返回
		//return SM_X509_CRT_PARSE_FAILED;
    }


    mbedtls_ssl_conf_ca_chain(conf, ca, NULL );
    mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED );
#endif

    if( mbedtls_ssl_setup(ssl, conf) != 0 )
    {
		return SM_SSL_SETUP_FAILED;
    }


#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if( mbedtls_ssl_set_hostname(ssl, node->host) != 0 )
    {
		return SM_HOSTNAME_FAILED;
    }
#endif



	server_fd->fd = node->soc_id;
	mbedtls_ssl_set_bio(ssl, server_fd, sm_tls_send, sm_tls_recv, NULL );

    if((result = mbedtls_ssl_handshake(ssl)) != 0)
    {
        if(result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
			return SM_SOCKET_WOULDBLOCK;
        }
		else
		{
			SM_PORITNG_LOG("[sm_soc] handshake failed %d...", result);
            return result;
        }
    }

	SM_PORITNG_LOG("[sm_soc] handshake success...");
	return SM_SOCKET_SUCCESS;
}

void sm_tls_free(tls_connect_t **tls)
{
	SM_PORITNG_LOG("[sm_soc] free node->tls: %x", (*tls));
	
	if(NULL == *tls)
		return;

    mbedtls_ssl_free(&(*tls)->ssl);
    mbedtls_ssl_config_free(&(*tls)->conf);
    mbedtls_ctr_drbg_free(&(*tls)->ctr_drbg);
    mbedtls_entropy_free(&(*tls)->entropy);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_free(&(*tls)->ca);
#endif

	sm_mfree(*tls);
	*tls = NULL;
}
#endif
