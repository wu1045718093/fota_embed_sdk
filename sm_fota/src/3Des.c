#include "3DES.h"
#include <stdlib.h>
#include <stdio.h>
//#include <memory.h>
#include <string.h>
#include <ctype.h>
#include "sm_debug.h"
#include "sm_system.h"
#include "sm_platform.h"


const char IP_Table[64] =
{
	58, 50, 42, 34, 26, 18, 10, 2,
	60, 52, 44, 36, 28, 20, 12, 4,
	62, 54, 46, 38, 30, 22, 14, 6,
	64, 56, 48, 40, 32, 24, 16, 8,
	57, 49, 41, 33, 25, 17,  9, 1,
	59, 51, 43, 35, 27, 19, 11, 3,
	61, 53, 45, 37, 29, 21, 13, 5,
	63, 55, 47, 39, 31, 23, 15, 7
};

const char IPR_Table[64] =
{
	40, 8, 48, 16, 56, 24, 64, 32,
	39, 7, 47, 15, 55, 23, 63, 31,
	38, 6, 46, 14, 54, 22, 62, 30,
	37, 5, 45, 13, 53, 21, 61, 29,
	36, 4, 44, 12, 52, 20, 60, 28,
	35, 3, 43, 11, 51, 19, 59, 27,
	34, 2, 42, 10, 50, 18, 58, 26,
	33, 1, 41,  9, 49, 17, 57, 25
};

const char E_Table[48] =
{
	32,  1,  2,  3,  4,  5,
	4,  5,  6,  7,  8,  9,
	8,  9, 10, 11, 12, 13,
	12, 13, 14, 15, 16, 17,
	16, 17, 18, 19, 20, 21,
	20, 21, 22, 23, 24, 25,
	24, 25, 26, 27, 28, 29,
	28, 29, 30, 31, 32,  1
};

const char P_Table[32] =
{
	16, 7, 20, 21,
	29, 12, 28, 17,
	1,  15, 23, 26,
	5,  18, 31, 10,
	2,  8, 24, 14,
	32, 27, 3,  9,
	19, 13, 30, 6,
	22, 11, 4,  25
};

const char PC1_Table[56] =
{
	57, 49, 41, 33, 25, 17,  9,
	1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27,
	19, 11,  3, 60, 52, 44, 36,
	63, 55, 47, 39, 31, 23, 15,
	7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29,
	21, 13,  5, 28, 20, 12,  4
};

const char PC2_Table[48] =
{
	14, 17, 11, 24,  1,  5,
	3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8,
	16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55,
	30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53,
	46, 42, 50, 36, 29, 32
};

const char LOOP_Table[16] =
{
	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

const char S_Box[8][4][16] =
{
	{
		{14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
		{ 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
		{ 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
		{15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13}
	},

	{
		{15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
		{ 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
		{ 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15},
		{13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9}
	},

	{
		{10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
		{13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
		{13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
		{ 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12}
	},

	{
		{ 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
		{13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9},
		{10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
		{ 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14}
	},

	{
		{ 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9},
		{14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
		{ 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
		{11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3}
	},

	{
		{12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11},
		{10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
		{ 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
		{ 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}
	},

	{
		{ 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
		{13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
		{ 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
		{ 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12}
	},

	{
		{13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
		{ 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2},
		{ 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
		{ 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11}
	}
};

char*  ch64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



static void ByteToBit(char *Out, const char *In, int bits);
static void BitToByte(char *Out, const char *In, int bits);
static void RotateL(char *In, int len, int loop);
static void Xor(char *InA, const char *InB, int len);
static void Transform(char *Out, const char *In, const char *Table, int len);
static void S_func(char Out[32], const char In[48]);
static void F_func(char In[32], const char Ki[48]);
static void SetSubKey(PSubKey pSubKey, const char Key[8]);
static void DES(char Out[8], const char In[8], const PSubKey pSubKey, int Type);



static void ByteToBit(char *Out, const char *In, int bits)
{
	int i;
	for (i=0; i<bits; ++i)
		Out[i] = (In[i>>3]>>(7 - i&7)) & 1;
}

static void BitToByte(char *Out, const char *In, int bits)
{
	int i;
	sm_memset(Out, 0, bits>>3);
	for (i=0; i<bits; ++i) Out[i>>3] |= In[i]<<(7 - i&7);
}

static void RotateL(char *In, int len, int loop)
{
	char *szTmp = NULL;

	szTmp = (char *)sm_malloc(256);
	if(szTmp == NULL)
		return;

	sm_memset(szTmp, 0, 256);

	if((loop == 0) || (loop >= 256)||(len >= 256))
	{
		sm_mfree(szTmp);
		return;
	}

	sm_memset(szTmp, 0x00, sizeof(szTmp));

	sm_memcpy(szTmp, In, loop);
	memmove(In, In+loop, len-loop);
	sm_memcpy(In+len-loop, szTmp, loop);

	sm_mfree(szTmp);
}


static void Xor(char *InA, const char *InB, int len)
{
	int i;
	for (i=0; i<len; ++i) InA[i] ^= InB[i];
}


static void Transform(char *Out, const char *In, const char *Table, int len)
{
	char *szTmp = NULL;
	int i;

	szTmp = (char *)sm_malloc(256);
	if(szTmp == NULL)
		return;

	sm_memset(szTmp, 0, 256);

	if((!Out) || (!In) || (!Table) ||(len >= 256))
	{
		sm_mfree(szTmp);
		return;
	}

	for (i=0; i<len; ++i) szTmp[i] = In[Table[i]-1];

	sm_memcpy(Out, szTmp, len);

	sm_mfree(szTmp);
}


static void S_func(char Out[32], const char In[48])
{
	int i,j,k,l;
	for (i=0,j=0,k=0; i<8; ++i,In+=6,Out+=4)
	{
		j = (In[0]<<1) + In[5];
		k = (In[1]<<3) + (In[2]<<2) + (In[3]<<1) + In[4];

		for ( l=0; l<4; ++l)
			Out[l] = (S_Box[i][j][k]>>(3 - l)) & 1;
	}
}


static void F_func(char In[32], const char Ki[48])
{
	char *MR = NULL;

	MR = (char *)sm_malloc(48);
	if(MR == NULL)
		return;

	sm_memset(MR, 0, 48);

	Transform(MR, In, E_Table, 48);
	Xor(MR, Ki, 48);
	S_func(In, MR);
	Transform(In, In, P_Table, 32);

	sm_mfree(MR);
}

static void SetSubKey(PSubKey pSubKey, const char Key[8])
{
	int i;
	char *K = NULL;
	char *KL =NULL;
	char *KR =NULL;
       //CPUartLogPrintf("enter %s:%x",__FUNCTION__,&i);

	K = (char *)sm_malloc(64);
	if(K == NULL)
		return;

	sm_memset(K, 0, 64);

	ByteToBit(K, Key, 64);

	Transform(K, K, PC1_Table, 56);

	KL=&K[0];
	KR=&K[28];

	for ( i=0; i<16; ++i)
	{
		RotateL(KL, 28, LOOP_Table[i]);
		RotateL(KR, 28, LOOP_Table[i]);
		Transform((*pSubKey)[i], K, PC2_Table, 48);
	}

	sm_mfree(K);
       //CPUartLogPrintf("leave %s",__FUNCTION__);
}

static void DES(char Out[8], const char In[8], const PSubKey pSubKey, int Type)
{
	int i;
	char *M = NULL;
	char *ML=NULL;
	char *MR=NULL;
	char *szTmp= NULL;
       //CPUartLogPrintf("enter %s",__FUNCTION__);

	M= (char *)sm_malloc(64);
	if(M == NULL)
		return;

	sm_memset(M, 0, 64);

	ByteToBit(M, In, 64);
	Transform(M, M, IP_Table, 64);

	ML=&M[0];
	MR=&M[32];
	

	szTmp= (char *)sm_malloc(32);
	if(szTmp== NULL)
	{
	       sm_mfree(M);
		return;
	}

	sm_memset(szTmp, 0, 32);

	if (Type == ENCRYPT)
	{
		for (i=0; i<16; ++i)
		{
			sm_memcpy(szTmp, MR, 32);
			F_func(MR, (*pSubKey)[i]);
			Xor(MR, ML, 32);
			sm_memcpy(ML, szTmp, 32);
		}
	}
	else
	{
		for (i=15; i>=0; --i)
		{
			sm_memcpy(szTmp, MR, 32);
			F_func(MR, (*pSubKey)[i]);
			Xor(MR, ML, 32);
			sm_memcpy(ML, szTmp, 32);
		}
	}
	RotateL(M, 64, 32);
	Transform(M, M, IPR_Table, 64);
	BitToByte(Out, M, 64);

	sm_mfree(M);
	sm_mfree(szTmp);

       //CPUartLogPrintf("leave %s",__FUNCTION__);
}


int Run1Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8])
{
	int i,j,k;

	char *m_SubKey = NULL;
	char cvec[8] = {0};
	char cvin[8] = {0};
       //CPUartLogPrintf("enter %s",__FUNCTION__);

	m_SubKey = (char *)sm_malloc(48<<4);    //16*48
	if(m_SubKey == NULL)
		return -1;

	sm_memset(m_SubKey, 0, 48<<4);


	if (!In || !Key || !Out) goto exit;

	if (key_len & 0x00000007) goto exit;

	if (in_len & 0x00000007) goto exit;

	if (out_len < in_len) goto exit;

	SetSubKey((PSubKey)m_SubKey, Key);

	if (bMode == ECB)
	{
		for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
		{
			DES(Out, In, (PSubKey)m_SubKey, bType);
		}
	}
	else if (bMode == CBC)
	{
		if (cvecstr == NULL) goto exit;

		//char cvec[8] = {0};
		//char cvin[8] = {0};

		sm_memcpy(cvec, cvecstr, 8);

		for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
		{
			if (bType == ENCRYPT)
			{
				for ( k=0; k<8; ++k)
				{
					cvin[k] = In[k] ^ cvec[k];
				}
			}
			else
			{
				sm_memcpy(cvin, In, 8);
			}

			DES(Out, cvin, (PSubKey)m_SubKey, bType);

			if (bType == ENCRYPT)
			{
				sm_memcpy(cvec, Out, 8);
			}
			else
			{
				for (k=0; k<8; ++k)
				{
					Out[k] = Out[k] ^ cvec[k];
				}
				sm_memcpy(cvec, cvin, 8);
			}
		}
	}
	else
	{
		goto exit;
	}

       sm_mfree(m_SubKey);

       //CPUartLogPrintf("leave %s",__FUNCTION__);
	return 1;

exit:
       sm_mfree(m_SubKey);
       return 0;
}


int Run3Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8])
{
	int i,j,k;
	unsigned char nKey;
	char *m_SubKey[3] = {NULL};
	char cvec[8] = {0};
	char cvin[8] = {0};
       //CPUartLogPrintf("enter %s:%x",__FUNCTION__,&i);

	m_SubKey[0] = (char *)sm_malloc(48<<4);    //16*48
	if(m_SubKey[0] == NULL)
		return -1;


	m_SubKey[1] = (char *)sm_malloc(48<<4);    //16*48
	if(m_SubKey[1] == NULL)
	{
		sm_mfree(m_SubKey[0]);
		return -1;
	}


	m_SubKey[2] = (char *)sm_malloc(48<<4);    //16*48
	if(m_SubKey[2] == NULL)
	{
		sm_mfree(m_SubKey[0]);
		sm_mfree(m_SubKey[1]);
		return -1;
	}

	sm_memset(m_SubKey[0], 0, 48<<4);       //16*48
	sm_memset(m_SubKey[1], 0, 48<<4);       //16*48
	sm_memset(m_SubKey[2], 0, 48<<4);       //16*48

       //CPUartLogPrintf("%s:in = %x, key = %x, out =%x",__FUNCTION__,In ,Key, Out);

	if (!In || !Key || !Out) 		goto exit;

	sm_memcpy(Out, In, (in_len+0x07)/0x08*0x08);

	if (in_len & 0x00000007) 		goto exit;
	if (key_len & 0x00000007) 		goto exit;
	if (out_len < in_len) 		goto exit;

	nKey = (key_len>>3)>3 ? 3 : (key_len>>3);
	for ( i=0; i<nKey; i++)
	{
		SetSubKey((PSubKey)m_SubKey[i], &Key[i<<3]);
	}

	if (bMode == ECB)
	{
		if (nKey ==	1)
		{
			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				DES(Out, In, (PSubKey)m_SubKey[0], bType);
			}
		}
		else if (nKey == 2)
		{
			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				DES(Out, In,  (PSubKey)m_SubKey[0], bType);
				DES(Out, Out, (PSubKey)m_SubKey[1], bType==ENCRYPT?DECRYPT:ENCRYPT);
				DES(Out, Out, (PSubKey)m_SubKey[0], bType);
			}
		}
		else if (nKey == 3)
		{
			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				DES(Out, In,  (PSubKey)m_SubKey[bType?2:0], bType);
				DES(Out, Out, (PSubKey)m_SubKey[1],         bType==ENCRYPT?DECRYPT:ENCRYPT);
				DES(Out, Out, (PSubKey)m_SubKey[bType?0:2], bType);
			}
		}
		else
		{
		       goto exit;
		}
	}
	else if (bMode == CBC)
	{
		if (cvecstr == NULL) 		goto exit;

		//char cvec[8] = {0};
		//char cvin[8] = {0};

		sm_memcpy(cvec, cvecstr, 8);

		if (nKey == 1)
		{
			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				if (bType == ENCRYPT)
				{
					for (k=0; k<8; ++k)
					{
						cvin[k]	= In[k] ^ cvec[k];
					}
				}
				else
				{
					sm_memcpy(cvin, In, 8);
				}

				DES(Out, cvin, (PSubKey)m_SubKey[0], bType);

				if (bType == ENCRYPT)
				{
					sm_memcpy(cvec, Out, 8);
				}
				else
				{
					for (k=0; k<8; ++k)
					{
						Out[k] = Out[k] ^ cvec[k];
					}
					sm_memcpy(cvec, cvin, 8);
				}
			}
		}
		else if (nKey == 2)
		{
			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				if (bType == ENCRYPT)
				{
					for ( k=0; k<8; ++k)
					{
						cvin[k] = In[k] ^ cvec[k];
					}
				}
				else
				{
					sm_memcpy(cvin, In, 8);
				}

				DES(Out, cvin, (PSubKey)m_SubKey[0], bType);
				DES(Out, Out,  (PSubKey)m_SubKey[1], bType==ENCRYPT?DECRYPT:ENCRYPT);
				DES(Out, Out,  (PSubKey)m_SubKey[0], bType);

				if (bType == ENCRYPT)
				{
					sm_memcpy(cvec, Out, 8);
				}
				else
				{
					for (k=0; k<8; ++k)
					{
						Out[k] = Out[k] ^ cvec[k];
					}
					sm_memcpy(cvec, cvin, 8);
				}
			}
		}
		else if (nKey == 3)
		{

			for (i=0,j=in_len>>3; i<j; ++i,Out+=8,In+=8)
			{
				if (bType == ENCRYPT)
				{
					for (k=0; k<8; ++k)
					{
						cvin[k]	= In[k] ^ cvec[k];
					}
				}
				else
				{
					sm_memcpy(cvin, In, 8);
				}

				DES(Out, cvin, (PSubKey)m_SubKey[bType?2:0], bType);
				DES(Out, Out,  (PSubKey)m_SubKey[1],         bType==ENCRYPT?DECRYPT:ENCRYPT);
				DES(Out, Out,  (PSubKey)m_SubKey[bType?0:2], bType);

				if (bType == ENCRYPT)
				{
					sm_memcpy(cvec, Out, 8);
				}
				else
				{
					for (k=0; k<8; ++k)
					{
						Out[k] = Out[k] ^ cvec[k];
					}
					sm_memcpy(cvec, cvin, 8);
				}
			}
		}
		else
		{
		       goto exit;
		}
	}
	else
	{
		goto exit;
	}

       sm_mfree(m_SubKey[0]);
       sm_mfree(m_SubKey[1]);
       sm_mfree(m_SubKey[2]);
       //CPUartLogPrintf("leave %s",__FUNCTION__);
	return 1;

exit:

       sm_mfree(m_SubKey[0]);
       sm_mfree(m_SubKey[1]);
       sm_mfree(m_SubKey[2]);
       return 0;
}


int	RunPad(int nType,const char* In,unsigned in_len,char* Out,int* padlen)
{
	int res = (in_len & 0x00000007);

	*padlen	=	((int)in_len+8-res);
	
	SM_PORITNG_LOG("[sm_3des] padlen: %d", *padlen);
	sm_memcpy(Out,In,in_len);

	if (nType	==	PAD_ISO_1)
	{
		sm_memset(Out+in_len,0x00,8-res);
	}
	else if (nType	==	PAD_ISO_2)
	{
		sm_memset(Out+in_len,0x80,1);
		sm_memset(Out+in_len,0x00,7-res);
	}
	else if (nType	==	PAD_PKCS_7)
	{
		sm_memset(Out+in_len,8-res,8-res);
	}
	else
	{
		return 0;
	}

	return 1;
}


void RunRsm(char *Text)
{
	int len,tmpint;

	len=strlen(Text);
	tmpint=*(Text+len-1);
	
	SM_PORITNG_LOG("[sm_3des] tmpint: %d", tmpint);
	*(Text+len-tmpint)=0x00;
}


int CovertKey(char *iKey, char *oKey)
{
	int	 inlen,i,j;
	unsigned char p,q,t,m,n;
	char *in = NULL, *out=NULL;

	in = (char *)sm_malloc(64);
	if(in == NULL)
		return -1;

	sm_memset(in, 0, 64);

	out = (char *)sm_malloc(64);
	if(out == NULL)
	{
		sm_mfree(in);
		return -1;
	}

	sm_memset(out, 0, 64);

	inlen=strlen(iKey);

	if (inlen!=48)
		goto exit;
	strcpy(in,iKey);

	for (i=0; i<inlen; i++)
	{
		if (!isxdigit(in[i]))
			goto exit;
	}
	for (i=0,j=0; i<inlen; i+=2,j++)
	{
		p=toupper(in[i]);
		q=toupper(in[i+1]);

		if (isdigit(p))
			m=p-48;
		else
			m=p-55;

		if (isdigit(q))
			n=q-48;
		else
			n=q-55;

		p=(char)((m<<4)&0xf0);
		q=n&0x0f;
		t=p|q;
		out[j]=t;
	}
	sm_memcpy(oKey,out,j+1);

       sm_mfree(in);
       sm_mfree(out);
	return 1;

exit:
       sm_mfree(in);
       sm_mfree(out);
	return 0;
}

char *Base64Encode(char *src,int srclen)
{
	int n,buflen,i,j;
	int pading=0;
	char *buf;
	static char *dst;

	buf=src;
	buflen=n=srclen;
	if (n%3!=0) /* pad with 0x00 by using a temp buffer */
	{
		pading=1;
		buflen=n+3-n%3;
		buf=(char*)sm_malloc(buflen+1);
		sm_memset(buf,0,buflen+1);
		sm_memcpy(buf,src,n);
		for (i=0; i<3-n%3; i++)
			buf[n+i]=0x00;
	}

	dst=(char*)sm_malloc(buflen*4/3+1);
	sm_memset(dst,0,buflen*4/3+1);
	for (i=0,j=0; i<buflen; i+=3,j+=4)
	{
		dst[j]=(buf[i]&0xFC)>>2;
		dst[j+1]=((buf[i]&0x03)<<4) + ((buf[i+1]&0xF0)>>4);
		dst[j+2]=((buf[i+1]&0x0F)<<2) + ((buf[i+2]&0xC0)>>6);
		dst[j+3]=buf[i+2]&0x3F;
	}

	for (i=0; i<buflen*4/3; i++)
	{
		dst[i]=ch64[dst[i]];
	}
	for (i=0; i<3-n%3; i++)
		dst[j-i-1]='=';

	if (pading)
		sm_mfree(buf);
	return dst;
}

char *Base64Decode(char *src)
{
	int m,n,i,j,len;
	char *p;
	static char *dst;
	char *strbuf = NULL;

	strbuf= (char *)sm_malloc(256);
	if(strbuf == NULL)
		return NULL;

	sm_memset(strbuf, 0, 256);

	if (src == 0 || src[0] == 0)
	{
		sm_mfree(strbuf);
		return NULL;
	}

	len = strlen(src);
	if (len % 4)
	{
		sm_mfree(strbuf);
		return NULL;
	}

	for (i = 0; i < len-2; i++)
	{
		if (src[i] == '=')
		{
		    sm_mfree(strbuf);
		    return NULL;
		}
	}

	strcpy(strbuf,src);
	n=strlen(src);
	for (i=0; i<n; i++)
	{
		p=strchr(ch64,src[i]);
		if (!p)
			break;
		src[i]=p-ch64;
	}
	dst=(char*)sm_malloc(n*3/4+1);
	sm_memset(dst,0,n*3/4+1);
	for (i=0,j=0; i<n; i+=4,j+=3)
	{
		dst[j]=(src[i]<<2) + ((src[i+1]&0x30)>>4);
		dst[j+1]=((src[i+1]&0x0F)<<4) + ((src[i+2]&0x3C)>>2);
		dst[j+2]=((src[i+2]&0x03)<<6) + src[i+3];
	}
	m=strcspn(strbuf,"=");
	for (i=0; i<n-m; i++)
		dst[j-i-1]=0x00;

	sm_mfree(strbuf);
	return dst;
}

void MyDesInit(int* cryptmode,int* padmode,char* cvec)
{
	char buf[10];

	sm_memset(buf,0,sizeof(buf));
	*cryptmode=CBC;
	*padmode=PAD_PKCS_7;
	buf[0]=0x01;
	buf[1]=0x02;
	buf[2]=0x03;
	buf[3]=0x04;
	buf[4]=0x05;
	buf[5]=0x06;
	buf[6]=0x07;
	buf[7]=0x08;
	sm_memcpy(cvec,buf,8);
}


int Crypt3Des(int type,char* in,char* key,char* out)
{

	int cryptmode,padmode,inlen,outlength,keylen,i;
	char cvec[10],instr[256],keystr[256],outstr[256],tmpstr[256];
	char* p,*q;

	sm_memset(cvec,0,sizeof(cvec));
	sm_memset(instr,0,sizeof(instr));
	sm_memset(outstr,0,sizeof(outstr));
	sm_memset(keystr,0,sizeof(keystr));
	sm_memset(tmpstr,0,sizeof(tmpstr));

	MyDesInit(&cryptmode,&padmode,cvec);
	inlen=strlen(in);

	if (in == 0 || in[0] == 0)
		return -7;

	inlen = strlen(in);


	if (inlen % 8)
		return -8;

	if (key == 0) return -9;

	keylen=strlen(key);
	if (keylen!=48)
		return -1;
	for (i = 0; i < keylen; i++)
	{
		if ((key[i] >= '0' && key[i] <= '9') || (key[i] >= 'a' && key[i] <= 'f') || (key[i] >= 'A' && key[i] <= 'F'))
			continue;
		else
			return -10;
	}
	if (inlen>256)
		return 0;
	if (!CovertKey(key,keystr))
		return -2;

	keylen=strlen(keystr);

	if (type==ENCRYPT)
	{

		if (!RunPad(padmode,in,strlen(in),instr,&inlen))
			return -3;
		if (!Run3Des(0,cryptmode,instr,inlen,keystr,(unsigned char)keylen,outstr,sizeof(outstr),cvec))
			return -4;
		outlength=strlen(outstr);

		p=Base64Encode(outstr,outlength);
		if (p==NULL)
			return -6;
		strcpy(out,p);
		sm_mfree(p);
		return 1;
	}
	else if (type==DECRYPT)
	{
		strcpy(tmpstr,in);
		q=Base64Decode(tmpstr);
		if (q==NULL)
			return -6;
		strcpy(instr,q);
		inlen=strlen(q);
		sm_mfree(q);

		if (!Run3Des(1,cryptmode,instr,inlen,keystr,(unsigned char)keylen,outstr,sizeof(outstr),cvec))
			return -4;
		RunRsm(outstr);
		strcpy(out,outstr);
	}
	else
		return -5;

	return 1;
}


unsigned char GetByte(char *s)
{
	int                v1;
	int                v2;

	if (s[0] >= '0' && s[0] <= '9')
		v1 = s[0] - '0';
	else if (s[0] >= 'a' && s[0] <= 'f')
		v1 = s[0] - 'a' + 10;
	else
		v1 = s[0] - 'A' + 10;

	if (s[1] >= '0' && s[1] <= '9')
		v2 = s[1] - '0';
	else if (s[1] >= 'a' && s[1] <= 'f')
		v2 = s[1] - 'a' + 10;
	else
		v2 = s[1] - 'A' + 10;

	return (v1*16+v2);
}





