#include <stdlib.h>
#include <stdio.h>


#include "sm_flash.h"
#include "sm_error.h"
#include "sm_system.h"
#include "sm_download.h"
#include "sm_debug.h"

//MTK头文件
#include "MMIDataType.h"
#include "syscomp_config.h"
#include "task_config.h"
#include "MMI_features.h"
#include "flash_disk.h"

#ifdef __NOR_SUPPORT_RAW_DISK__
#ifndef WIN32
extern kal_int32 sm_write_buffer(kal_uint32 addr,kal_uint8* data_ptr, kal_uint32 len);
extern kal_uint32 sm_erase_disk(void);
extern kal_int32  sm_read_buffer(kal_uint32 addr, kal_uint8* data_ptr, kal_uint32 len);
extern kal_uint32 sm_get_block_index(kal_uint32 addr);
extern  void sm_erase_block(kal_uint32 blkIdx);
#else
kal_int32 sm_write_buffer(kal_uint32 addr,kal_uint8* data_ptr, kal_uint32 len){return 0;}
kal_uint32 sm_erase_disk(void){return 	0;}
void sm_erase_block(kal_uint32 blkIdx) {}
kal_int32  sm_read_buffer(kal_uint32 addr, kal_uint8* data_ptr, kal_uint32 len){return 0;}
kal_uint32 sm_get_block_index(kal_uint32 addr) {return 0};
#endif
#endif


/***************************局部函数*****************************************/



sm_u32 sm_addr_to_block(sm_u32 addr)
{
   #ifdef __NOR_SUPPORT_RAW_DISK__ 
   return sm_get_block_index(addr);
   #endif
}


/**********************************************************************************************
NAME	 : sm_flash_read
FUCNTION : 读取flash中的数据
PARAMETER: addr---flash起始地址 buf---存放读取数据的BUF, len--要读取的长度
RETURN	 : SM_ERR_OK---读取成功			其他值---读取失败
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_flash_read(sm_u32 address, sm_u8 *destination, sm_u32 size)
{
	sm_s32 result;
#ifdef __NOR_SUPPORT_RAW_DISK__ 
   	result = sm_read_buffer(address, destination,size); 
	if(result == RAW_DISK_ERR_NONE)
	{		
		SM_PORITNG_LOG("%s success at addr[%d], len[%d]\n\r", __FUNCTION__, address, size);
		return SM_ERR_OK;
	}
	else
	{	
		SM_PORITNG_LOG("%s error at addr[%d], len[%d], ret[%d]\n\r", __FUNCTION__, address, size, result);
		return SM_ERR_FLASH_READ;
	}
#endif

	return(SM_ERR_FLASH_READ);
}




/**********************************************************************************************
NAME	 : sm_flash_write
FUCNTION : 往flash写数据
PARAMETER: addr---flash起始地址 buf---要写入的数据, len--要写入的长度
RETURN	 : SM_ERR_OK---写入成功			其他值---写入失败
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_flash_write(sm_u32 address,
                       sm_u8 *source,
                       sm_u32 size)
{
	sm_s32 result;
	#ifdef __NOR_SUPPORT_RAW_DISK__
	result=sm_write_buffer(address,source,size);
	if(result == RAW_DISK_ERR_NONE)
	{		
		SM_PORITNG_LOG("%s success at addr[%d], len[%d]\n\r", __FUNCTION__, address, size);
		return SM_ERR_OK;
	}
	else
	{	
		SM_PORITNG_LOG("%s error at addr[%d], len[%d], ret[%d]\n\r", __FUNCTION__, address, size, result);
		return SM_ERR_FLASH_WRITE;
	}
	#endif	

	return SM_ERR_FLASH_WRITE;
}


/**
函数说明：flash初始化 
参数说明：无
返回值：成功sm_ERR_OK（值为0），其它为失败
*/
sm_s32 sm_flash_init()
{
	sm_s32 result;

#ifdef __NOR_SUPPORT_RAW_DISK__
 	result=sm_erase_disk();
	if(result == RAW_DISK_ERR_NONE)
		return SM_ERR_OK;
	else
		return SM_ERR_FLASH_INIT;
#else
	return SM_ERR_FLASH_INIT;		
#endif


}

/**
函数说明：获取当前flash的一个page的大小 
参数说明：无
返回值：成功返回page的size，失败返回-1 
*/
sm_s32 sm_flash_getPageSize()
{
	return SM_PAGE_SIZE;
}

/**
函数说明：flash初始化 
参数说明：无
返回值：成功SM_ERR_OK（值为0），其它为失败
*/
sm_s32 sm_flash_getBlockSize()
{
	return SM_BLOCK_SIZE;
}



/**********************************************************************************************
NAME	 : sm_get_ota_update_addr
FUCNTION : 获取模组OTA升级固件存放的起始地址
PARAMETER: flash_add---固件地址 flash_len---固件长度
RETURN	 : SM_ERR_OK---存在待更新固件			其他值---获取失败
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_get_ota_update_addr(sm_u32 *flash_add, sm_u32 *flash_len)
{
	sm_u32 ret;
	char len_buf[SM_FLASH_OTA_UPDATE_LEN] = {0};

	
	if(NULL == flash_add || NULL == flash_len)
	{
		return SM_ERR_INVALID_PARAM;
	}
	
	*flash_add = SM_FLASH_OTA_UPDATE_ADDR;
	ret = sm_flash_read(SM_FLASH_OTA_UPDATE_LEN_ADDR, len_buf, SM_FLASH_OTA_UPDATE_LEN);
	
	if(SM_ERR_OK == ret)
	{
		sm_memcpy(flash_len, len_buf, SM_FLASH_OTA_UPDATE_LEN);
		SM_PORITNG_LOG("[sm_flash] ota len: %d", *flash_len);
	}

	return ret;
}

/**********************************************************************************************
NAME	 : sm_get_ota_backup_addr
FUCNTION : 获取模组OTA升级备份存放的起始地址
PARAMETER: flash_add---固件地址 flash_len---固件长度
RETURN	 : SM_ERR_OK---存在待更新固件			其他值---获取失败
DESCRIP. :
AUTHOR	 : wteng
**********************************************************************************************/
sm_s32 sm_get_ota_backup_addr(sm_u32 *flash_add, sm_u32 *flash_len)
{
	if(NULL == flash_add || NULL == flash_len)
	{
		return SM_ERR_INVALID_PARAM;
	}
	*flash_add = SM_FLASH_OTA_BACKUP_ADDR;
	*flash_len = SM_FLASH_OTA_BACKUP_SIZE;
	
	return SM_ERR_OK;
}


/**
函数说明：擦除block 
参数说明：addr需要擦除的block的地址
返回值：成功SM_ERR_OK（值为0），其它为失败
*/
sm_s32 sm_flash_eraseBlock(sm_u32 addr)
{
	sm_u32 block_idx;

	block_idx = sm_addr_to_block(addr);
#ifdef __NOR_SUPPORT_RAW_DISK__
 	sm_erase_block(block_idx);
#endif
	return SM_ERR_OK;
}

