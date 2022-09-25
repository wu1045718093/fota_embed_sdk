#ifndef _ARCH_INTERFACE_H_
#define _ARCH_INTERFACE_H_

#include "7zTypes.h"
#include "LZMA_decoder.h"



/****************************macro configure*****************************/
//#define  RUN_ON_PC				//仅限于在PC上调试
#define  ENABLE_DEBUG			//


/****************************flash configure****************************/
#define F_SECTORE_SIZE			0x1000		
#define F_SECTOR_MASK			0xFFFF1000	
#define F_ALIGN_SECTOR(addr)	((addr)&F_SECTOR_MASK)

#define F_MAX_FW_SIZE			0xC00000 					
#define F_BACKUP_SIZE			0x20000	
#define F_FOR_STATUS_SIZE		F_SECTORE_SIZE				
#define F_DOWN_SIZE				0x20000	
#define MAX_TOPATCH_BLOCK_NUM	5000 		

#define F_BK_POOL_NUM			((F_MAX_FW_SIZE+F_SECTORE_SIZE)/F_SECTORE_SIZE)


#if defined(RUN_ON_PC)
#define FW_TO_PATCH_ADDR		0x01000000
#define FW_SOURCE_ADDR			0x02000000
#define FW_BACKUP_ADDR			0x03000000
#endif

#define F_MAX_BK_SECTOR_NUM 	((F_BACKUP_SIZE-F_FOR_STATUS_SIZE)/F_SECTORE_SIZE)




#define IN_BUF_SIZE 			F_SECTORE_SIZE
#define OUT_BUF_SIZE 			F_SECTORE_SIZE

#define STATE_OK				0

#define BACKUP_MAGIC			0x82562647
#define BACKUP_MAGIC_SIZE		4
#define BACK_FLASH_STATUS_U		0xffff	
#define BACK_FLASH_STATUS_S		0xffa5	
#define BACK_FLASH_STATUS_C		0xa5a5	

#define BACKUP_BUSY				0x1		
#define BACKUP_FREE				0x00	




#define MIN(x,y) 				(((x)<(y)) ? (x) : (y))



#define LZMA_OK                   0
#define LZMA_ERROR_DATA           1
#define LZMA_ERROR_MEM            2
#define LZMA_ERROR_CRC            3
#define LZMA_ERROR_UNSUPPORTED    4
#define LZMA_ERROR_PARAM          5
#define LZMA_ERROR_INPUT_EOF      6
#define LZMA_ERROR_OUTPUT_EOF     7
#define LZMA_ERROR_READ           8
#define LZMA_ERROR_WRITE          9
#define LZMA_ERROR_PROGRESS       10
#define LZMA_ERROR_FAIL           11
#define LZMA_ERROR_THREAD         12
#define LZMA_ERROR_ARCHIVE        16
#define LZMA_ERROR_NO_ARCHIVE     17



typedef struct {
    void *(*Alloc)(void *p, uint32_t size);
    void (*Free)(void *p, void *address);   /* address can be 0 */
} lzma_alloc_t;


typedef struct _back_map_status_t_{
	uint16_t block_status;
	uint16_t sea_block_num;
}back_status_t;

//alloc 4K nv memory to store this struct array
typedef struct diff_ctrl_str{
	int32_t diff;
	int32_t adjust;
}diff_ctrl_t;

typedef struct flash_handle_str{
	int (*write)(uint32_t start_address, uint8_t *buf, uint32_t length);
    int (*read)(uint32_t start_address, uint8_t *buf, uint32_t length);
    int (*erase)(uint32_t start_address, uint32_t size);
} flash_handle_t;

typedef struct flash_ttt{
	uint32_t firmware_addr;
	uint32_t firmware_size;
	uint32_t download_addr;
	uint32_t download_size;
	uint32_t backup_addr;
	uint32_t backup_size;
	uint32_t sector_size;
}flash_des_t;


typedef struct _ant_plan_des_t_{
	/***flash handle***/
	flash_handle_t *phandle;

	//endian flag
	uint32_t big_endian;

	/***flash configure***/
	uint32_t f_fw_addr;			
	uint32_t f_dw_addr;			
	uint32_t f_max_fw_size;		
	uint32_t f_sector_size;		

	/****topatch block information****/
	uint32_t b_total;			
	uint32_t b_max;				
	uint32_t b_status_addr;		
	uint32_t b_curr_block;		
	diff_ctrl_t b_diff_ctrl[MAX_TOPATCH_BLOCK_NUM];
												

	/*****process*****/
	int32_t p_newpos;			
	int32_t p_oldpos;			

	/****backup sea information****/
	uint32_t bk_sea_addr;		
	uint32_t bk_max_size;		
	uint32_t bk_status_addr;	
	uint32_t bk_magic_addr;		
	uint32_t bk_max_sector;		
	uint32_t bk_map_used;		
	uint32_t bk_used_oldest;	
	back_status_t bk_r_status[F_BK_POOL_NUM];
	uint8_t bk_map[F_MAX_BK_SECTOR_NUM];	
}ant_plan_t;


typedef struct decode_man_str{
	CLzmaDec state;
	uint32_t src_start;	
	uint32_t src_curr;
	uint32_t src_end;	
	uint32_t unpackSize;

	//decode buffer struct
	Byte *inbuf;
	Byte *outbuf;
	uint32_t inbuf_size;
	uint32_t outbuf_size;
	uint32_t in_size;
	uint32_t in_pos;
//	uint32_t out_pos;
}decode_f_man;

#endif
