#ifndef _SM_FOTA_H_
#define _SM_FOTA_H_

#include "sm_datatype.h"

#define BLOCK_SIZE	0x8000
#ifdef __SERIAL_FLASH_EN__
#define NOR_PAGE_SIZE             (512)         //512 bytes
#else
#define NOR_PAGE_SIZE             (2048)        //2048 bytes
#endif

#define BL_FLASH_OTA_UPDATE_SECTOR_NUM			16							//模组升级
#define BL_FLASH_OTA_BACKUP_SECTOR_NUM			16							//升级备份数据
#define BL_FLASH_OTA_UPDATE_LEN       			sizeof(unsigned int)		//用一个字节存放升级文件的长度
#define BL_FLASH_OTA_UPDATE_STATUS_LEN       	sizeof(unsigned int)		//用一个字节保存OTA升级包状态

#define BL_FLASH_OTA_UPDATE_LEN_ADDR		(0x800000 - BL_FLASH_OTA_UPDATE_LEN)
#define BL_FLASH_OTA_UPDATE_STATUS_ADDR		(BL_FLASH_OTA_UPDATE_LEN_ADDR - BL_FLASH_OTA_UPDATE_STATUS_LEN)

#define BL_FLASH_OTA_UPDATE_SIZE			(BL_FLASH_OTA_UPDATE_SECTOR_NUM*BLOCK_SIZE-BL_FLASH_OTA_UPDATE_LEN-BL_FLASH_OTA_UPDATE_STATUS_LEN)
#define BL_FLASH_OTA_UPDATE_ADDR			(BL_FLASH_OTA_UPDATE_STATUS_ADDR - BL_FLASH_OTA_UPDATE_SIZE)

#define BL_FLASH_OTA_BACKUP_SIZE			(BL_FLASH_OTA_BACKUP_SECTOR_NUM*BLOCK_SIZE)
#define BL_FLASH_OTA_BACKUP_ADDR			(BL_FLASH_OTA_UPDATE_ADDR - BL_FLASH_OTA_BACKUP_SIZE)


typedef enum 
{
	UPSTATE_IDLE , 
	UPSTATE_CHECK, /* 验签已通过 */
	UPSTATE_UPDATE, /* update */
	UPSTATE_UPDATEED,/* update finish */
}mi_updateState;


void mi_iot_update_moudle(void);
void wt_bl_debug_print(void* ctx, const char* fmt, ...);
mi_bool mi_ua_addrToBlockPage(uint32_t addr, uint32_t *vBlock, uint32_t *vPage);
unsigned int mi_bl_read_block(unsigned char* dest, unsigned int start, unsigned int size);
unsigned int mi_bl_write_block(unsigned char* src, unsigned int start, unsigned int size);
unsigned int mi_bl_read_flash(unsigned int addr,unsigned char* data_ptr, unsigned int len);
unsigned int mi_bl_disk_read(unsigned int address, uint8_t *destination, unsigned int size);
unsigned int mi_bl_disk_write(unsigned int address, uint8_t *source,unsigned int size);
unsigned int mi_bl_disk_earse(uint32_t addr, uint32_t size);

int mi_iot_bootloader_earse(uint32_t start_address, uint32_t size);

#endif

