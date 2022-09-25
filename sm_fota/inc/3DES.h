#ifndef	_3DES__H_
#define _3DES__H_

#ifdef __cplusplus
extern "C"
{
#endif

#define ENCRYPT	0
#define DECRYPT	1

#define ECB	0
#define CBC	1

#define KEY_LEN_8 8
#define KEY_LEN_16 16
#define KEY_LEN_24 24

#define PAD_ISO_1	0
#define PAD_ISO_2	1
#define PAD_PKCS_7	2

	typedef char (*PSubKey)[16][48];

	void RunRsm(char* Text);
	int	CovertKey(char* iKey, char *oKey);
	int	RunPad(int nType,const char* In,unsigned in_len,char* Out,int* padlen);
	int Run1Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8]);
	int Run3Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8]);
	//int Crypt3Des(int type,char* in,char* key,char* out);

	char *Base64Encode(char *src, int srclen);
	char *Base64Decode(char *src);
	unsigned char GetByte(char *s);

#ifdef __cplusplus
}
#endif

#endif



