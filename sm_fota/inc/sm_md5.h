 #ifndef _SM_MD5_H_
 #define _SM_MD5_H_

/* typedef a 32 bit type */
typedef unsigned int UINT4;

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
  UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
  UINT4 buf[4];                                    /* scratch buffer */
  unsigned char in[64];                              /* input buffer */
  unsigned char digest[16];     /* actual digest after MD5Final call */
} SM_MD5_CTX;

void SM_MD5Init (SM_MD5_CTX *mdContext);
void SM_MD5Update (SM_MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
void SM_MD5Final (SM_MD5_CTX *mdContext);
void SM_Transform (UINT4 *buf, UINT4 *in);

/*
 **********************************************************************
 ** End of md5.h                                                     **
 ******************************* (cut) ********************************
 */
 
 #endif