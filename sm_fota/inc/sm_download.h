#ifndef __SM_DOWNLOAD_H__
#define __SM_DOWNLOAD_H__

#include "sm_system.h"
#include "sm_socket.h"

#define SM_URI_MAX_LEN     512
#define SM_RECV_BUF        1460
#define SM_MD5_LEN         32



/* 打印宏 */
#define MSG_DEBUG   0x01
#define MSG_INFO    0x02
#define MSG_ERROR   0x04



#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define SM_HTTP_BREAK			206
#define SM_HTTP_SUCCESS			200
#define SM_HTTP_REDIRECT		302


#define SM_HTTP_FLAG_RESULT           0x00000001
#define SM_HTTP_FLAG_DATALEN          0x00000002
#define SM_HTTP_FLAG_HEADLEN          0x00000004
#define SM_HTTP_FLAG_ENDTAG           0x00000008


#define SM_FOTA_STATE_DOWNLOADING	"downloading"
#define SM_FOTA_STATE_DONE			"done"
#define SM_FOTA_STATE_FAIL			"fail"


#define SM_HTTP_RETRY_COUNT			10
#define SM_HTTP_RETRY_INTERVAL		(60*1000)


enum 
{
	UPSTATE_IDLE , 
	UPSTATE_CHECK, /* 验签已通过 */
	UPSTATE_UPDATE, /* update */
	UPSTATE_UPDATEED,/* update finish */
};

typedef enum {
	SM_FOTA_OK = 0,
	SM_FOTA_DOWNLOAD_TIMEOUT = -1,		//下载超时
	SM_FOTA_FILE_NOT_FOUND = -2,      	//文件不存在
	SM_FOTA_MD5_ERROR = -3,    			//MD5校验失败
	SM_FOTA_UPDATE_ERROR = -4,    		//固件更新失败	
	SM_FOTA_BIN_TOO_BIG = -5,    		//下载包太大
}SM_FOTA_ERROR_CODE;
	

typedef enum {
	SADT_STATE_IDLE = 0,
	SADT_STATE_READY,
	SADT_STATE_DODNS,      // 正在做DNS解析
	SADT_STATE_DNSDONE,    // 域名解析完毕
	SADT_STATE_SOCOPEN,    // socket已创建
	SADT_STATE_CONNTING,   // 正在连接socket
	SADT_STATE_ESTABLISH,  // 连接已建立
    SADT_STATE_SEND_REQ,   // 发送POST/GET请求
    SADT_STATE_WAIT_ACK,   // 等待POST/GET回复
    SADT_STATE_TRANSING,   // 文件数据块传输
    SADT_STATE_WAIT_FINISH,// 等待传输结束信号
	SADT_STATE_ERROR,	   // 出错状态
} SADT_STATE_E;


typedef struct {
	sm_u8      success;    // 是否解析成功
	sm_u8      completed;  // 头部是否完整
	sm_u8      verno10;    // 是否是HTTP1.0的版本
	sm_u8      reserved;   // 保留
	
	sm_s32      flags;
    sm_s32      result;    // 200, 302, 404, 500
    sm_s32      dataLen;
	sm_s32      headLen;
}HTTP_RSP_HEAD;


typedef struct {
	sm_u32 bin_len;					//bin文件的总大小
	sm_u32 cur_len;					//当前已下载的大小
    int port;                       //主机端口号
    char host_name[HOST_NAME_LEN];  //主机名
    char uri[SM_URI_MAX_LEN];          //资源路径
    char buffer[SM_RECV_BUF];          //读写缓冲
    char md5[SM_MD5_LEN];
   	char bin_type;					//固件包类型 0--MCU的bin文件	1--模组的bin文件
   	char mode;						//连接类型		 0--HTTP	1--HTTPS
    SADT_STATE_E state;				//下载状态
    SADT_STATE_E retry;				//重试次数
} sm_download_t;

sm_bool sm_sys_get_updatefile_md5_string(sm_s8* md5String);
#endif
