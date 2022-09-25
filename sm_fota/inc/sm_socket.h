#ifndef SM_SOCKET_H
#define SM_SOCKET_H

#include "sm_datatype.h"
#include "MMI_include.h"

#if defined(MBEDTLS_SUPPORT)
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"
//#include "mbedtls/net_sockets.h"
#endif


typedef sm_s8 sm_socket_handle;

#define HOST_NAME_LEN   256


enum
{
	SM_SOC_MODE = 0,
	SM_TLS_MODE = 1,	
};

#if defined(MBEDTLS_SUPPORT)
typedef struct{
	//tls
	mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ssl_session saved_session;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt ca;
#endif
}tls_connect_t;
#endif


typedef struct
{
	sm_s32 dns_id;
	sm_s32 handshake_timer;
	sm_u8  status;
}sm_connect_struct;

typedef struct
{
	sm_s32 app_id;
	sm_s32 port;
	sm_connect_struct connect;
	sm_socket_handle soc_id;
	void *head;
	void *next;
	sm_s32 (*ReadSocket)(sm_socket_handle soc_id);
	sm_s32 (*WriteSocket)(sm_socket_handle soc_id);
	void (*ConnectSocket)(sm_socket_handle soc_id);
	void (*CloseSocket)(sm_socket_handle soc_id);
#if defined(MBEDTLS_SUPPORT)
	tls_connect_t *tls;
	sm_u32	handshake_timer;
#endif
	sm_u8 mode;
	char host[HOST_NAME_LEN];
}sm_socket_list_struct;


enum
{
	SM_SOCKET_SUCCESS = 0,
	SM_INVALID_SOCKET = -1,
	SM_SOCKET_WOULDBLOCK = -2,
	SM_CTR_DRBG_SEED_FAILED = -3,
	SM_SSL_CONFIG_DEFAULTS_FAILED = -4,
	SM_SSL_SETUP_FAILED = -5,
	SM_HOSTNAME_FAILED = -6,
	SM_X509_CRT_PARSE_FAILED = -7,
	SM_SSL_HANDSHAKE_FAILED = -8,
	SM_BAD_INPUT = -9,
	SM_MALLOC_TLS_FAILED = -10,
};

void sm_socket_set_netid(sm_u8 netid);
sm_u8 sm_socket_get_netid();


sm_bool sm_soc_message_dispatch(void *sig_ptr);
void sm_socket_connect(sm_socket_handle soc_id);
void sm_socket_close(sm_socket_handle soc_id);
void sm_socket_read(sm_socket_handle soc_id);
void sm_socket_write(sm_socket_handle soc_id);


#if defined(MBEDTLS_SUPPORT)
sm_s32 sm_tls_handshake_timeout(void *data);
sm_s32 sm_tls_handshake(sm_socket_list_struct *node);
void sm_tls_free(tls_connect_t **tls);
extern void mbedtls_ssl_init( mbedtls_ssl_context *ssl );
extern void mbedtls_ssl_config_init( mbedtls_ssl_config *conf );
extern void mbedtls_entropy_init( mbedtls_entropy_context *ctx );
extern int mbedtls_ctr_drbg_seed( mbedtls_ctr_drbg_context *ctx,
                   int (*f_entropy)(void *, unsigned char *, size_t),
                   void *p_entropy,
                   const unsigned char *custom,
                   size_t len );
extern int mbedtls_ssl_config_defaults( mbedtls_ssl_config *conf,
                                 int endpoint, int transport, int preset );
extern void mbedtls_ssl_conf_rng( mbedtls_ssl_config *conf,
                  int (*f_rng)(void *, unsigned char *, size_t),
                  void *p_rng );
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
extern int mbedtls_ssl_conf_psk( mbedtls_ssl_config *conf,
                const unsigned char *psk, size_t psk_len,
                const unsigned char *psk_identity, size_t psk_identity_len );
#endif
extern void mbedtls_ssl_free( mbedtls_ssl_context *ssl );
extern void mbedtls_ssl_config_free( mbedtls_ssl_config *conf );
extern void mbedtls_ctr_drbg_free( mbedtls_ctr_drbg_context *ctx );
extern void mbedtls_entropy_free( mbedtls_entropy_context *ctx );
#if defined(MBEDTLS_X509_CRT_PARSE_C)
extern void mbedtls_x509_crt_free( mbedtls_x509_crt *crt );
extern void mbedtls_x509_crt_init( mbedtls_x509_crt *crt );
#endif
#endif

#endif

