#ifndef __SM_FLASH_H__
#define __SM_FLASH_H__

#include "sm_datatype.h"

#define SM_BLOCK_SIZE	0x1000
#ifdef __SERIAL_FLASH_EN__
#define SM_PAGE_SIZE             (512)         //512 bytes
#else
#define SM_PAGE_SIZE             (2048)        //2048 bytes
#endif

//总大小应跟FLASH_SPLOAD_SECTOR_NUM一致
#define SM_FLASH_OTA_UPDATE_SECTOR_NUM			16					//模组升级
#define SM_FLASH_OTA_BACKUP_SECTOR_NUM			16					//升级备份数据
#define SM_FLASH_OTA_UPDATE_LEN       			sizeof(sm_u32)		//用一个字节存放升级文件的长度
#define SM_FLASH_OTA_UPDATE_STATUS_LEN       	sizeof(sm_u32)		//用一个字节保存OTA升级包状态
	

#define SM_FLASH_OTA_UPDATE_LEN_ADDR		(0x100000 - SM_FLASH_OTA_UPDATE_LEN)							//固件长度存放地址
#define SM_FLASH_OTA_UPDATE_STATUS_ADDR		(SM_FLASH_OTA_UPDATE_LEN_ADDR - SM_FLASH_OTA_UPDATE_STATUS_LEN)	//升级状态地址


#define SM_FLASH_OTA_UPDATE_SIZE	(SM_FLASH_OTA_UPDATE_SECTOR_NUM*SM_BLOCK_SIZE - SM_FLASH_OTA_UPDATE_LEN - SM_FLASH_OTA_UPDATE_STATUS_LEN)	//升级包最大长度
#define SM_FLASH_OTA_UPDATE_ADDR	(SM_FLASH_OTA_UPDATE_STATUS_ADDR - SM_FLASH_OTA_UPDATE_SIZE)			//升级包存放起始地址

#define SM_FLASH_OTA_BACKUP_SIZE	(SM_FLASH_OTA_BACKUP_SECTOR_NUM*SM_BLOCK_SIZE)							//恢复包存放最大长度
#define SM_FLASH_OTA_BACKUP_ADDR	(SM_FLASH_OTA_UPDATE_ADDR - SM_FLASH_OTA_BACKUP_SIZE)					//备份包存放起始地址

#endif
