/************************************************************
Copyright (C), 2016, Leon, All Rights Reserved.
FileName: download.c
coding: UTF-8
Description: 实现简单的http下载功能
Author: Leon
Version: 1.0
Date: 2016-12-2 10:49:32
Function:

History:
<author>    <time>  <version>   <description>
 Leon

************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sm_download.h"
#include "sm_main.h"
#include "sm_flash.h"
#include "sm_debug.h"
#include "sm_md5.h"
#include "sm_system.h"





sm_download_t *sm_download_info  = NULL;
extern const char *httpTagString_CRLF           = "\r\n";
extern const char *httpTagString_Get            = "GET";
extern const char *httpTagString_Post           = "POST";
extern const char *httpTagString_Http           = "HTTP";
extern const char *httpTagString_Http10         = "HTTP/1.0";
extern const char *httpTagString_Http11         = "HTTP/1.1";
extern const char *httpTagString_Host           = "Host";

extern const char *httpTagString_Connection     = "Connection";  // keep-alive, close
extern const char *httpTagString_KeepAlive      = "Keep-Alive";
extern const char *httpTagString_Close          = "Close";

extern const char *httpTagString_ContentLength  = "Content-Length";
extern const char *httpTagString_ContentType    = "Content-Type";

static const sm_s8 dec2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void sm_http_retry();

static sm_socket_list_struct sm_http = {0};
sm_socket_list_struct *sm_pHttpNode = &sm_http;

/**********************************************************************************************
NAME	 : sm_check_ota_addr_valid
FUCNTION : 检查OTA地址是否越界
PARAMETER: offset---地址偏移, len--要写入的长度
RETURN	 : SM_ERR_OK---合法的地址		SM_ERR_SPACE_NOT_ENGHOU---超过限制了
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_check_ota_addr_valid(sm_u32 offset, sm_u32 len)
{
	if( offset > SM_FLASH_OTA_UPDATE_SIZE || 
		offset + len > SM_FLASH_OTA_UPDATE_SIZE)
		return SM_ERR_SPACE_NOT_ENGHOU;

		
	return SM_ERR_OK;
}

/**********************************************************************************************
NAME	 : sm_write_ota_bin
FUCNTION : 保存OTA升级包数据
PARAMETER: offset---地址偏移 buf---要写入的数据, len--要写入的长度
RETURN	 : SM_ERR_OK---写入成功			其他值---写入失败
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_write_ota_bin(sm_u32 offset, const sm_u8 *buffer, sm_u32 len)
{
	sm_s32 ret = FALSE;

	if(NULL == buffer || 0 == len)
	{
		return SM_ERR_INVALID_PARAM;
	}

	ret = sm_check_ota_addr_valid(offset, len);

	if(SM_ERR_OK != ret)
	{
		return ret;
	}
	
	ret = sm_flash_write(SM_FLASH_OTA_UPDATE_ADDR + offset, buffer, len);

	if (ret != SM_ERR_OK)
		return SM_ERR_FLASH_WRITE;
		
	return SM_ERR_OK;
}

void sm_flash_erase_fota_area()
{
	sm_u32 i;

	for(i = 0; i < SM_FLASH_OTA_UPDATE_SECTOR_NUM; i++)
	{
		sm_flash_eraseBlock(SM_FLASH_OTA_UPDATE_ADDR + i*SM_BLOCK_SIZE);
	}
}


/* 不区分大小写的strstr */

int sm_strncasecmp(const char* pszStr1, const char* pszStr2, int len)
{
    while (--len >=0 && toupper((unsigned char)*pszStr1) == toupper((unsigned char)*pszStr2++))
    	if (*pszStr1++ == '\0') return 0;
    return (len<0? 0: toupper((unsigned char)*pszStr1) - toupper((unsigned char)*--pszStr2));
}

char *sm_strncasestr(char *str, char *sub)
{
	int len = 0;
    if(!str || !sub)
        return NULL;

    len = strlen(sub);
    if (len == 0)
    {
        return NULL;
    }

    while (*str)
    {
        if (sm_strncasecmp(str, sub, len) == 0)
        {
            return str;
        }
        ++str;
    }
    return NULL;
}

// HTTP/1.1 200 OK
// Date: Thu, 26 Mar 2015 13:01:21 GMT
// X-Powered-By: Express
// Accept-Ranges: bytes
// Cache-Control: public, max-age=0
// Last-Modified: Thu, 26 Mar 2015 07:15:47 GMT
// ETag: W/"13e6-3829749211"
// Content-Type: application/octet-stream
// Content-Length: 5094
// X-Via: 1.1 ccsskan113017:8101 (Cdn Cache Server V2.0)
//Connection: keep-alive
//
// HTTP/1.0 302 Found
// Cache-Control: no-cache
// Connection: close
// Location: http://183.57.28.31/hipanda.hf.openstorage.cn/talkres/audio/45092000.mp3?wsiphost=local
// 
// HTTP/1.1 200 OK
// Date: Thu, 28 Apr 2016 07:33:46 GMT
// Server: Nginx-XF/1.9.2-001
// Content-Type: audio/mpeg
// Content-Length: 20041
// Accept-Ranges: bytes
// Last-Modified: Mon, 12 Oct 2015 08:29:00 GMT
// ETag: 0fc74c3f8a8149fe5cab8355068140cd
// X-Timestamp: 1444638539.30042
// X-Trans-Id: tx98a5a7bed3464a6aa7d04-005721bcda
// Via: 1.1 yzh26:8104 (Cdn Cache Server V2.0), 1.1 mzh31:8500 (Cdn Cache Server V2.0)
// Connection: keep-alive
//
void sm_http_parse_head(HTTP_RSP_HEAD *h, char *buffer, sm_s32 len)
{
	char *p;
	sm_s32 t;
	const char *vernostr;

	memset(h, 0, sizeof(HTTP_RSP_HEAD));
	p = (char *)sm_strstr(buffer, httpTagString_Http11);
	if (p)
	{
		vernostr = httpTagString_Http11;
		h->verno10 = SM_FALSE;
	}
	else
	{
		vernostr = httpTagString_Http10;
		p = (char *)sm_strstr(buffer, vernostr);
		if (p)
		{		
			h->verno10 = SM_TRUE;
		}
	}

	if (p)
	{
		p += strlen(vernostr);
		while (' ' == *p || ':' == *p)
			p++;
	
		if (1 == sscanf(p, "%d", &t))
		{
			char *ts, *hs;

			hs = p;
			h->success = SM_TRUE;
			
			h->result = t;
			h->flags |= SM_HTTP_FLAG_RESULT;

			ts = p;
			p = (char *)strstr(hs, httpTagString_ContentLength);
			if (p)
			{
				p += strlen(httpTagString_ContentLength);
				while (' ' == *p || ':' == *p)
					p++;
		
				if (1 == sscanf(p, "%d", &t))
				{
					h->dataLen = t;
					h->flags |= SM_HTTP_FLAG_DATALEN;
				}

				if (ts < p)
					ts = p;
			}
			
			
			p = strstr(ts, "\r\n\r\n");
			if (p)
			{
				h->headLen = p - buffer + 4;
				h->flags |= SM_HTTP_FLAG_HEADLEN;
				h->completed = SM_TRUE;
			}

		}
	}
	
	SM_PORITNG_LOG("[sm_down] parse HTTP head: %d, %d, %d, 0x%x", h->result, h->headLen, h->dataLen, h->flags);
}


/* 解析URL, 成功返回0，失败返回-1 */
/* http://127.0.0.1:8080/testfile */
int sm_parser_url(char *url, sm_download_t *info)
{
    char *tmp = url, *start = NULL, *end = NULL;
    int len = 0;

    /* 跳过http:// */
    if(sm_strncasestr(tmp, "https://"))
    {   
        tmp += strlen("https://");
		info->mode = SM_TLS_MODE;
    }
	else if(sm_strncasestr(tmp, "http://"))
	{
		tmp += strlen("http://");
		info->mode = SM_SOC_MODE;
	}
	else
	{
		SM_PORITNG_LOG("[sm_down] url head error!\n");
		return SM_ERR_INVALID_PARAM;	  
	}
	
    start = tmp;
    if(!(tmp = strchr(start, '/')))
    {
        SM_PORITNG_LOG("[sm_down] url invaild\n");
        return SM_ERR_INVALID_PARAM;      
    }
    end = tmp;

    /*解析端口号和主机*/
	if(info->mode == 0)
	{
    	info->port = 80;   	//http默认端口为80
	}
	else
	{
    	info->port = 443;	//https默认端口为443
	}

    len = MIN(end - start, HOST_NAME_LEN - 1);
    strncpy(info->host_name, start, len);
    info->host_name[len] = '\0';

    if((tmp = strchr(start, ':')) && tmp < end)
    {
        info->port = atoi(tmp + 1);
        if(info->port <= 0 || info->port >= 65535)
        {
            SM_PORITNG_LOG("[sm_down] url port invaild\n");
            return SM_ERR_INVALID_PARAM;
        }
        /* 覆盖之前的赋值 */
        len = MIN(tmp - start, HOST_NAME_LEN - 1);
        strncpy(info->host_name, start, len);
        info->host_name[len] = '\0';
    }

    /* 复制uri */
    start = end;
    strncpy(info->uri, start, SM_URI_MAX_LEN - 1);

    SM_PORITNG_LOG("[sm_down] parse url ok host:%s, port:%d, uri:%s\n", 
        info->host_name, info->port, info->uri);
    return SM_ERR_OK;
}

sm_s32 sm_package_http_quest(sm_download_t *info, char *out_buf, sm_u32 buf_len, sm_s32 *real_len)
{
	if(NULL == info || NULL == out_buf || NULL == real_len)
	{
		return SM_ERR_INVALID_PARAM;
	}

	if(info->cur_len > 0)
	{
		*real_len = snprintf(out_buf, buf_len,
				"GET %s HTTP/1.1\r\n"
				"Host: %s:%d\r\n"
				"Range: bytes=%d-\r\n"
				"Connection: Keep-Alive\r\n"
				"Accept: */*\r\n"
				"\r\n",
				info->uri,
				info->host_name,
				info->port,
				info->cur_len
				);
	}
	else
	{
		*real_len = snprintf(out_buf, buf_len,
				"GET %s HTTP/1.1\r\n"
				"Host: %s:%d\r\n"
				"Connection: Keep-Alive\r\n"
				"Accept: */*\r\n"
				"\r\n",
				info->uri,
				info->host_name,
				info->port
				);
	}


	return SM_ERR_OK;
}


sm_s32 sm_extract_ip_addr(sm_s8* szDstBuf, const sm_s8* szSrcURL, sm_u32 *nHostPort )
{
	int ret = 0;
	char portBuf[10];
	char* portPtr;

	// check "http" 
	if(memcmp(szSrcURL, "http://", 7) == 0)
	{
		szSrcURL += 7;
	}
	else if(memcmp(szSrcURL, "https://", 8) == 0)
	{
		szSrcURL += 8;
		ret = 1;
	}

	// copy data 
	while(*szSrcURL != 0 && *szSrcURL != ':' && *szSrcURL != '/')
	{
		*szDstBuf++ = *szSrcURL++;
	}
	*szDstBuf = 0;

	if(*szSrcURL == ':')
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
		port = atoi(portBuf);
		*nHostPort = port;
	}
	else
	{
		*nHostPort = 80;
	}

	return ret;
}

void sm_download_socket_read_wait_timeout(void *data)
{
	sm_u8 persent;
	SM_PORITNG_LOG("[sm_down] %s ", __FUNCTION__);

	persent = sm_download_info->cur_len*100/sm_download_info->bin_len;
	sm_download_report(SM_FOTA_STATE_FAIL, persent, SM_FOTA_DOWNLOAD_TIMEOUT);
	sm_socket_close((sm_socket_handle)data);

}

void sm_download_socket_write_wait_timeout(void *data)
{
	SM_PORITNG_LOG("[sm_down] %s ", __FUNCTION__);
	sm_socket_close((sm_socket_handle)data);
}

void sm_download_socket_connect_timeout(void *data)
{
	SM_PORITNG_LOG("[sm_down] %s ", __FUNCTION__);
	sm_socket_close((sm_socket_handle)data);
}


void sm_http_connect_error(void)
{
	if(sm_download_info->retry <= SM_HTTP_RETRY_COUNT)
	{
		wt_start_timer(SM_HTTP_RETRY_TIMER_ID, SM_HTTP_RETRY_INTERVAL, sm_http_retry);
	}
	else
	{
		sm_mfree(sm_download_info);
		sm_download_info = NULL;
	}

}


void sm_download_close(sm_socket_handle soc_id)
{
	sm_socket_close(soc_id);
}


sm_s32 sm_http_write(sm_socket_handle soc_id)
{
	sm_s32 nwrite = 0;
	sm_s32 sock_error_code;
	sm_s32 real_len = 0, offset = 0;
	char send_buf[512] = {0};

	if(SM_ERR_OK ==  sm_package_http_quest(sm_download_info, send_buf, 512, &real_len));
	{
		do
		{
			nwrite = sm_socket_send_api(sm_pHttpNode, send_buf, real_len);
		
			if(nwrite > 0)
			{
				offset += nwrite;
			}
			else
			{
				if(0 == nwrite)
				{
					//阻塞掉了,等待系统回调即可,很少出现这种情况
					wt_start_timer_ex(	SM_HTTP_CONNECT_TIMER_ID,
											15*1000,
											sm_download_socket_write_wait_timeout,
											(void *)soc_id);
					break;
				}
				else
				{
					//写入错误,在这里处理
					//关闭socket
					sm_download_close(soc_id);
					return nwrite;
				}
			}
		
		}while(offset < real_len);
		
	}


	if(offset >= real_len)
	{
		//如果是请求数据(服务器需回复)
		//开个15s的定时器等待回复
		wt_start_timer_ex( SM_HTTP_CONNECT_TIMER_ID,
							15*1000,
							sm_download_socket_connect_timeout,
							(void *)soc_id);
	}
	
	sm_download_info->state = SADT_STATE_WAIT_ACK;

	return nwrite;
}

void sm_http_connect(sm_socket_handle soc_id)
{
	wt_stop_timer(SM_HTTP_CONNECT_TIMER_ID);
	sm_http_write(soc_id);
}

void sm_http_close(sm_socket_handle soc_id)
{
	wt_stop_timer(SM_HTTP_CONNECT_TIMER_ID);
	sm_pHttpNode->soc_id = SM_INVALID_SOCKET;
	sm_http_connect_error();
}

sm_s32 sm_http_read(sm_socket_handle soc_id)
{
	sm_s32 ret = 0;
	sm_s32 left_len = 0;

	//收到消息关闭定时器
	wt_stop_timer(SM_HTTP_CONNECT_TIMER_ID);
	//StopTimer(SN_TIMER_SOC_ACK);
	memset(sm_download_info->buffer, 0x00, SM_RECV_BUF);

	//这种写法是不安全的
	//当socket数据大于SM_RECV_BUF的时候会导致之后的数据全部无法收到
	//暂时先这样使用,以后出现大数据的情况再做修改
	do
	{
		ret = sm_socket_recv_api(sm_pHttpNode, sm_download_info->buffer, SM_RECV_BUF, &left_len);
		if(ret > 0)
		{
			SM_PORITNG_LOG("[sm_down] Read len: %d left len: %d Read data: %s", ret, left_len, sm_download_info->buffer);
		}
		else
		{

			SM_PORITNG_LOG("[sm_down] Read data error: %d\r\n", ret);
			if(ret == 0)
			{
				//数据不足一个record，继续等待,同时开启一个定时器防止网络异常导致无法再进入
				wt_start_timer_ex(	SM_HTTP_CONNECT_TIMER_ID,
										15*1000,
										sm_download_socket_read_wait_timeout,
										(void *)soc_id);
				return ret;
			}
			else
			{
				//发生错误了 关闭SOCKET
				sm_download_close(soc_id);
				return ret;
			}
			
			return ret;
		}

		
		switch(sm_download_info->state)
		{
			case SADT_STATE_WAIT_ACK:
			{
				HTTP_RSP_HEAD head;
				sm_u32 max_len;
				
				// 解析HTTP回复的头部数据
				sm_http_parse_head(&head, sm_download_info->buffer, ret);
				if(head.dataLen > SM_FLASH_OTA_UPDATE_SIZE)
				{
					//升级包超过限制
					SM_PORITNG_LOG("[sm_down] patch too big\r\n");					
					sm_download_report(SM_FOTA_STATE_FAIL, 0, SM_FOTA_BIN_TOO_BIG);					
					sm_download_info->retry = SM_HTTP_RETRY_COUNT+1;
					sm_download_close(soc_id);
					return SM_ERR_SPACE_NOT_ENGHOU;
				}
				
				switch (head.result)
				{
				case SM_HTTP_SUCCESS:
					{
						char bin_len[4] = {0};
						
						// 如果头很长, 这一次没有传输完成怎么办, 那处理就麻烦了
						sm_download_info->state = SADT_STATE_TRANSING;
						sm_memcpy(bin_len, &head.dataLen, sizeof(sm_s32));
						sm_download_info->bin_len = head.dataLen;
						sm_download_info->cur_len += ret - head.headLen;

						sm_flash_write(SM_FLASH_OTA_UPDATE_LEN_ADDR, bin_len, sizeof(sm_s32));
						sm_write_ota_bin(0, sm_download_info->buffer + head.headLen, ret - head.headLen);

						if(sm_download_info->cur_len == sm_download_info->bin_len)
						{
							//可能差分包很小不到1K
							char md5_buf[SM_MD5_LEN + 1] = {0};
							
							
							sm_sys_get_updatefile_md5_string(md5_buf);
							if(0 == sm_strcmp(md5_buf, sm_download_info->md5))
							{
							
								sm_u32 status = UPSTATE_CHECK;
								char status_buf[4] = {0};
								
								//将OTA固件标志位置1
								sm_memcpy(status_buf, &status, sizeof(sm_s32));
								sm_flash_write(SM_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, sizeof(sm_s32));
								SM_PORITNG_LOG("[sm_down] BIN verify OK");
								sm_download_report(SM_FOTA_STATE_DOWNLOADING, 100, SM_FOTA_OK);

								wt_start_timer(SM_RESTART_TIMER_ID, 10*1000, sm_restart);
								
							}
							else
							{
								//校验失败擦除数据通知服务器								
								SM_PORITNG_LOG("[sm_down] BIN verify error");
								sm_download_report(SM_FOTA_STATE_DONE, 100, SM_FOTA_MD5_ERROR);
								sm_download_info->retry = SM_HTTP_RETRY_COUNT+1;
								sm_download_close(soc_id);
								
							}
						}

						break;
					}
				case SM_HTTP_BREAK:
					{						
						// 如果头很长, 这一次没有传输完成怎么办, 那处理就麻烦了
						sm_download_info->state = SADT_STATE_TRANSING;

						sm_write_ota_bin(sm_download_info->cur_len, sm_download_info->buffer + head.headLen, ret - head.headLen);
						sm_download_info->cur_len += ret - head.headLen;
						
						if(sm_download_info->cur_len == sm_download_info->bin_len)
						{
							//剩余差分包很小不到1K
							char md5_buf[SM_MD5_LEN + 1] = {0};
							
							
							sm_sys_get_updatefile_md5_string(md5_buf);
							if(0 == sm_strcmp(md5_buf, sm_download_info->md5))
							{
							
								sm_u32 status = UPSTATE_CHECK;
								char status_buf[4] = {0};
								
								//将OTA固件标志位置1
								sm_memcpy(status_buf, &status, sizeof(sm_s32));
								sm_flash_write(SM_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, sizeof(sm_s32));
								SM_PORITNG_LOG("[sm_down] BIN verify OK");
								sm_download_report(SM_FOTA_STATE_DOWNLOADING, 100, SM_FOTA_OK);

								wt_start_timer(SM_RESTART_TIMER_ID, 10*1000, sm_restart);
								
							}
							else
							{
								//校验失败擦除数据通知服务器
								sm_download_report(SM_FOTA_STATE_DONE, 100, SM_FOTA_MD5_ERROR);
								sm_download_info->retry = SM_HTTP_RETRY_COUNT+1;
								sm_download_close(soc_id);
								SM_PORITNG_LOG("[sm_down] BIN verify error");
								
							}
						}

						break;
					}
				case SM_HTTP_REDIRECT:
					//考虑安全问题对于重定向的HTTP请求不做处理
					break;
				default:
					// 出错了等待socket被remote close吧
					return;
				}
			}
				break;
			case SADT_STATE_WAIT_FINISH:
				break;
				
			case SADT_STATE_TRANSING:
			{
				sm_bool wait;



				sm_write_ota_bin(sm_download_info->cur_len, sm_download_info->buffer, ret);
				sm_download_info->cur_len += ret;
				wait = (sm_bool)(sm_download_info->cur_len == sm_download_info->bin_len);
				SM_PORITNG_LOG("[sm_down]:Done size: %d. Total size: %d", sm_download_info->cur_len, sm_download_info->bin_len);			
				if(wait)
				{
					char md5_buf[SM_MD5_LEN + 1] = {0};


					sm_sys_get_updatefile_md5_string(md5_buf);
					if(0 == sm_strcmp(md5_buf, sm_download_info->md5))
					{

						sm_u32 status = 1;
						char status_buf[4] = {0};
						
						//将MCU固件标志位置1
						sm_memcpy(status_buf, &status, sizeof(sm_s32));
						sm_flash_write(SM_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, sizeof(sm_s32));
						SM_PORITNG_LOG("[sm_down] BIN verify OK");

						sm_restart();
						
					}
					else
					{
						//校验失败擦除数据通知服务器						
						sm_download_report(SM_FOTA_STATE_DONE, 100, SM_FOTA_MD5_ERROR);
						sm_download_info->retry = SM_HTTP_RETRY_COUNT+1;
						sm_download_close(soc_id);
						SM_PORITNG_LOG("[sm_down] BIN verify error");
					}
					
				}
			}
				break;
				
			default:
				SM_PORITNG_LOG("L:%d. upload, state=%d. error state.", __LINE__, sm_download_info->state);
				break;
		}	
	}while(sm_download_info->cur_len < sm_download_info->bin_len);

	return ret;

}

void sm_http_retry()
{
	sm_s32 ret;

	SM_PORITNG_LOG("sm_http_retry: %d",sm_download_info->retry);
	sm_download_info->retry++;
	
	ret = sm_socket_creat(sm_pHttpNode);
	if(SM_SOCKET_WOULDBLOCK == ret)
	{
		//如果是请求数据(服务器需回复)
		//开个20s的定时器等待回复
		wt_start_timer_ex( SM_HTTP_CONNECT_TIMER_ID,
							20*1000,
							sm_download_socket_connect_timeout,
							(void *)sm_pHttpNode->soc_id);

	}
	else
	{
		if(sm_download_info->retry <= SM_HTTP_RETRY_COUNT)
		{
			wt_start_timer(SM_HTTP_RETRY_TIMER_ID, SM_HTTP_RETRY_INTERVAL, sm_http_retry);
		}
		else
		{
			sm_mfree(sm_download_info);
			sm_download_info = NULL;
		}
	}	
}

sm_s32 sm_http_start(char *url, char *md5)
{
	sm_s32 ret, len;


	if(NULL == url || NULL == md5)
	{
		return SM_ERR_INVALID_PARAM;
	}

	//正在下载中不在接受下载命令
	if(NULL != sm_download_info)
	{
		SM_PORITNG_LOG("[sm_down] Download busy!");
		return SM_ERR_DOWNLOAD_BUSY;
	}


	sm_download_info = (sm_download_t *)sm_malloc(sizeof(sm_download_t));
	if(NULL == sm_download_info)
	{
		return SM_ERR_MALLOC_MEM_FAILE;
	}
	memset(sm_download_info, 0x00, sizeof(sm_download_t));

	len = (sm_strlen(md5) > SM_MD5_LEN) ? SM_MD5_LEN : sm_strlen(md5);
	sm_memcpy(sm_download_info->md5, md5, len);
		
	ret = sm_parser_url(url, sm_download_info);
	if(ret != SM_ERR_OK)
	{
		sm_mfree(sm_download_info);
		sm_download_info = NULL;
		return SM_ERR_INVALID_URL;
	}
	

	sm_pHttpNode->soc_id = SM_INVALID_SOCKET;	
    sm_pHttpNode->port = sm_download_info->port;
	sm_pHttpNode->mode = sm_download_info->mode;
	sm_pHttpNode->ConnectSocket = sm_http_connect;
	sm_pHttpNode->ReadSocket = sm_http_read;
	sm_pHttpNode->WriteSocket = sm_http_write;
	sm_pHttpNode->CloseSocket = sm_http_close;
#if defined(MBEDTLS_SUPPORT)	
	sm_pHttpNode->handshake_timer = SM_HTTP_HANDSHAKE_TIMER_ID;
#endif
	sm_memcpy(sm_pHttpNode->host, sm_download_info->host_name, HOST_NAME_LEN);


	ret = sm_socket_creat(sm_pHttpNode);
	if(SM_SOCKET_WOULDBLOCK == ret)
	{
		//如果是请求数据(服务器需回复)
		//开个20s的定时器等待回复
		wt_start_timer_ex( SM_HTTP_CONNECT_TIMER_ID,
							20*1000,
							sm_download_socket_connect_timeout,
							(void *)sm_pHttpNode->soc_id);

	}
	else
	{
		SM_PORITNG_LOG("[sm_main] Creat socket failed %d", ret);
	}

	sm_flash_erase_fota_area();
	
	return SM_ERR_OK;
}

/**
函数说明：获取已经下载差分升级包的md5值
参数说明：deltaSize 升级包的长度
md5String 升级包的md5的值
返回值：SM_TRUE表示成功，SM_FALSE表示失败 
*/
sm_bool sm_sys_get_updatefile_md5_string(sm_s8* md5String)
{
	SM_MD5_CTX md5Ctx;
	sm_u8 buf[SM_PAGE_SIZE];
	sm_s32 readSize = 0;
	unsigned char digest[16] = {0};
	
	sm_u32 deltaSize = 0;
	sm_u32 flashAddr = 0;
	sm_s32 i = 0;
	
	SM_MD5Init (&md5Ctx);
	
	sm_get_ota_update_addr(&flashAddr, &deltaSize);
	
	while(readSize < deltaSize)
	{
		sm_s32 ret = 0;
		ret = sm_flash_read(flashAddr + readSize, buf, SM_PAGE_SIZE);
		if (ret != SM_ERR_OK)
			return SM_FALSE;

		if (deltaSize - readSize > SM_PAGE_SIZE)
		{
			SM_MD5Update (&md5Ctx, buf, SM_PAGE_SIZE);
			readSize += SM_PAGE_SIZE;
		}
		else
		{
			SM_MD5Update (&md5Ctx, buf, deltaSize - readSize);
			readSize = deltaSize;
		}
	}

	SM_MD5Final (&md5Ctx);

	for (i=0; i<16; i++) 
	{
		md5String[i * 2] = dec2hex[(md5Ctx.digest[i] & 0xF0) >> 4];
		md5String[i * 2 + 1] = dec2hex[md5Ctx.digest[i] & 0x0F];
	}
	md5String[32] = 0;

	SM_PORITNG_LOG("[sm_down] calc md5: %s!", md5String);
	return SM_TRUE;
}

