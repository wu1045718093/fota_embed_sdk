////////////////////////////////////////////////////////////////////  
//                          _ooOoo_                               //  
//                         o8888888o                              //  
//                         88" . "88                              //  
//                         (| ^_^ |)                              //  
//                         O\  =  /O                              //  
//                      ____/`---'\____                           //  
//                    .'  \\|     |//  `.                         //  
//                   /  \\|||  :  |||//  \                        //  
//                  /  _||||| -:- |||||-  \                       //  
//                  |   | \\\  -  /// |   |                       //  
//                  | \_|  ''\---/''  |   |                       //  
//                  \  .-\__  `-`  ___/-. /                       //  
//                ___`. .'  /--.--\  `. . ___                     //  
//              ."" '<  `.___\_<|>_/___.'  >'"".                  //  
//            | | :  `- \`.;`\ _ /`;.`/ - ` : | |                 //  
//            \  \ `-.   \_ __\ /__ _/   .-` /  /                 //  
//      ========`-.____`-.___\_____/___.-`____.-'========         //  
//                           `=---='                              //  
//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        //  
//			  佛祖保佑	    			      永无BUG				      //  
////////////////////////////////////////////////////////////////////  

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "bl_common.h"
#include "FTL.h"
#include "kal_release.h"
#include "flash_opt.h"
#include "bl_custom.h"
#include "bl_MTK_BB_REG.H"
#include "custom_img_config.h"


#include "sm_fota.h"
#include "arch_interface.h"


#ifdef __NOR_SUPPORT_RAW_DISK__
#include "flash_disk.h"
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

#ifdef __LCD_DRIVER_IN_BL__
static mi_bool lcd_inited = mi_false;
#endif /* __LCD_DRIVER_IN_BL__ */

extern FTL_FuncTbl *g_ftlFuncTbl;
flash_des_t mi_flash_info = {0};
flash_handle_t flash_api;


extern int firmware_patch(flash_handle_t *f_handle, flash_des_t *flash_info);
extern void mi_iot_get_working_buffer();
extern uint8_t * earse_buf;



void wt_bl_debug_print(void* ctx, const char* fmt, ...) 
{
#if 0
	va_list ap;
	va_start (ap, fmt);
	bl_print_internal(LOG_INFO, fmt, ap);
	va_end (ap);
	bl_print(LOG_INFO, "\r"); 
#endif	

}

void wt_bl_DL_InitLCD()
{	
#if defined(__LCD_DRIVER_IN_BL__)

   if(!lcd_inited)
   {
      wt_bl_debug_print(NULL, "Init LCD\n\r");
      BL_LCDHWInit();
      BL_ShowUpdateFirmwareInitBackground();
      BL_LCDSetBackLight();

      {
         DCL_HANDLE rtc_handler;
         DCL_HANDLE pw_handle;

         rtc_handler = DclRTC_Open(DCL_RTC,FLAGS_NONE);
         DclRTC_Control(rtc_handler, RTC_CMD_SETXOSC, (DCL_CTRL_DATA_T *)NULL);
         DclRTC_Control(rtc_handler, RTC_CMD_HW_INIT, (DCL_CTRL_DATA_T *)NULL);
         DclRTC_Close(rtc_handler);

         pw_handle=DclPW_Open(DCL_PW, FLAGS_NONE);
         DclPW_Control(pw_handle,PW_CMD_POWERON,NULL);
         DclPW_Close(pw_handle);
      }

      lcd_inited = KAL_TRUE;

   }

#endif /* __LCD_DRIVER_IN_BL__ */
}


void wt_bl_progress(int32_t percent)
{
	wt_bl_debug_print(NULL, "ENTER adups_progress current = %d\n\n\r", percent);

	WacthDogRestart();

	if(percent <= 100)
	{
#if defined(__LCD_DRIVER_IN_BL__)
	  wt_bl_DL_InitLCD();
	  BL_ShowUpdateFirmwareProgress(percent);
#endif // __LCD_DRIVER_IN_BL__ 

	}
}


void wt_bl_ShowUpateSuccess(void)
{
#if defined(__LCD_DRIVER_IN_BL__)
   BL_ShowUpdateFirmwareOK();
#endif /* __LCD_DRIVER_IN_BL__ */
}


/**
 * extend to erase block index on NOR Flash Raw disk
 *
 * @param
 * @param[vBlock]	: block index to erase
 *
 * @remarks
 *
 * @return
 * rs_true	: erase successfully
 * rs_false	: erase failed
 */
mi_bool mi_ua_flash_erase_ext(uint32_t vBlock)
{
	//rs_trace("%s, vBlock is %p, \n\r", __FUNCTION__, vBlock);
	if (g_ftlFuncTbl->FTL_EraseBlock(vBlock, NULL) != FTL_SUCCESS) {
		wt_bl_debug_print(NULL, "%s finished, failed causion:erase error at block[%d], quit\n\r", __FUNCTION__, vBlock);
		return (mi_false);
	}
	return(mi_true);
}


/**
 * transfer physical address to block/page
 *
 * @param
 * @param[addr]		: physical address to transfer
 * @param[vBlock]	: data ptr of transferred block index
 * @param[vPage]	: data ptr of transferred page index
 *
 * @remarks
 *
 * @return
 * mi_true	: transfer successfully
 * mi_false	: transfer failed
 */
mi_bool mi_ua_addrToBlockPage(uint32_t addr, uint32_t *vBlock, uint32_t *vPage)
{
	uint32_t status;

	status = g_ftlFuncTbl->FTL_AddrToBlockPage(addr, vBlock, vPage, NULL);
	if (status != FTL_SUCCESS) {
		wt_bl_debug_print(NULL, "%s, transfer address[%x] to block/page failed, quit\n\r", __FUNCTION__, addr);
		return (mi_false);
	}

	return (mi_true);
}


mi_bool mi_bl_write_flash(uint32_t address,
                       uint8_t *source,
                       uint32_t size)
{
	uint32_t vBlock, vPage;
	uint32_t vAddress = ((uint32_t)address);//note, maybe error
	uint8_t *p;
	uint32_t addr;
	uint32_t blocksize ,blk_page_count;
	uint32_t needWriteSize = size;

	blocksize = g_ftlFuncTbl->FTL_GetBlockSize(0, NULL);
	blk_page_count = blocksize/NOR_PAGE_SIZE;

	//wt_bl_debug_print(NULL, "%s, address[%x], size[%d]\n\r", __FUNCTION__, vAddress, size);
//	rs_print_data_array(source, 4);
	// check page aligned or not
	if (vAddress & (NOR_PAGE_SIZE - 1)) {
		wt_bl_debug_print(NULL, "%s, failed causion:address is not aligned by NOR_PAGE_SIZE[0x200], quit\n\r", __FUNCTION__);
		return (mi_false);
	}

	// transfer address to Block/Page
	if (!mi_ua_addrToBlockPage(vAddress, &vBlock, &vPage)) {
		wt_bl_debug_print(NULL, "%s, failed causion:transfer address to block/page failed, quit\n\r", __FUNCTION__);
		return (mi_false);
	}

	//wt_bl_debug_print(NULL, "%s, transfer address[%p] to block[%d] page[%d], size is %x\n\r", __FUNCTION__, address, vBlock, vPage, size);

	// write data to special block/page cycle
	p = source;
	while (needWriteSize > NOR_PAGE_SIZE) {
		if (vPage > (blk_page_count - 1)) {
			vBlock++;
			vPage = 0;
		}

		
		if (g_ftlFuncTbl->FTL_WritePage(vBlock, vPage, (uint32_t *)p, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s finished, failed causion:erase error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}

		needWriteSize -= NOR_PAGE_SIZE;
		p += NOR_PAGE_SIZE;
		//handle page
		vPage++;
		
	}

	if (needWriteSize > 0)
	{
		uint8_t buf[NOR_PAGE_SIZE];

		memset(buf, 0xff, NOR_PAGE_SIZE);
		memcpy(buf,  p,  needWriteSize);
		if (g_ftlFuncTbl->FTL_WritePage(vBlock, vPage, (uint32_t *)buf, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s finished, failed causion:erase error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}		
	}
	
	return(mi_true);
}



mi_bool rs_ua_flash_read(uint8_t *destination,
                       unsigned int address,
                       unsigned int size)
{
	uint32_t vBlock, vPage, i, j, vSizeCount, vSizeRemain, vAlignOffset;
	uint32_t vAddress = ((uint32_t)address);
	uint8_t *p;
	uint8_t data[NOR_PAGE_SIZE] = {0};
	uint32_t blocksize ,blk_page_count;

	blocksize = g_ftlFuncTbl->FTL_GetBlockSize(0, NULL);
	blk_page_count = blocksize/NOR_PAGE_SIZE;
	
	//wt_bl_debug_print(NULL, "%s, vAddress[%x], size[%d]\n\r", __FUNCTION__, vAddress, size);
	// check some common error
	if (size < 0) {
		return (mi_false);
	}

	memset(data, 0x00, NOR_PAGE_SIZE);

	// check page aligned or not
	vAlignOffset = 0;
	if (vAddress & (NOR_PAGE_SIZE - 1)) {
		vAlignOffset = vAddress & (NOR_PAGE_SIZE - 1);
		vAddress = (vAddress - vAlignOffset);//vAddress & 0xfffffe00
	}

	//wt_bl_debug_print(NULL, "%s, handled vAddress[%x], vAlignOffset[%d]\n\r", __FUNCTION__, vAddress, vAlignOffset);
	// transfer address to Block/Page
	if (!mi_ua_addrToBlockPage(vAddress, &vBlock, &vPage)) {
		wt_bl_debug_print(NULL, "%s No-aligned route, failed causion:transfer address to block/page failed, quit\n\r", __FUNCTION__);
		return (mi_false);
	}

	// caculate size count/remain
	vSizeCount = (size / NOR_PAGE_SIZE);
	vSizeRemain = (size % NOR_PAGE_SIZE);

	//wt_bl_debug_print(NULL, "%s, transfer vAddress[%x] to vBlock[%d] vPage[%d], size[%d], vAlignOffset[%x]\
	//	vSizeCount[%d], vSizeRemain[%d]\n\r", __FUNCTION__, vAddress, vBlock, vPage, size, vAlignOffset, vSizeCount, vSizeRemain);

	// Section A: size < NOR_PAGE_SIZE (include Address aligned or not)
	if (vSizeCount == 0) {
//		wt_bl_debug_print(NULL, "%s, run Section A\n\r", __FUNCTION__);
		p = destination;//pointer to destination ptr
		// read one page data first
		if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s, Section A failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}

		// NO-aligned address
		if (vAlignOffset != 0) {
			// A.1 read size in one page, copy directly
			if ((NOR_PAGE_SIZE - vAlignOffset) >= vSizeRemain) {
				for (i = vAlignOffset; i < (vAlignOffset + vSizeRemain); i++) {
					*p = data[i];
					p++;
				}
//				wt_bl_debug_print(NULL, "%s, Section A.1 read data[0~3]:%x, data[%d~%d]:%x\n\r", __FUNCTION__, *(uint32_t *)destination, (size - 1), (size - 4), *(uint32_t *)(destination + size - 4));
			//	wt_bl_debug_print(NULL, "%s, Section A.1 read data[0~3]:", __FUNCTION__);
				//rs_print_data_array(destination, 4);
				//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
				//rs_print_data_array((destination + size - 4), 4);
				return (mi_true);
			// A.2 read size between two pages
			} else {
				vSizeRemain = (vSizeRemain - (NOR_PAGE_SIZE - vAlignOffset));
				wt_bl_debug_print(NULL, "%s, Section A.2 read size overlap two pages, remain size[%d]", __FUNCTION__, vSizeRemain);
				for (i = vAlignOffset; i < NOR_PAGE_SIZE; i++) {
					*p = data[i];
					p++;
				}
				// handle and check page
				vPage++;
				if (vPage > (blk_page_count - 1)) {
					vBlock++;
					vPage = 0;
					// check block count
				}
				// init data array
				memset(data, 0x00, NOR_PAGE_SIZE);
				
				// read one page
				if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
					wt_bl_debug_print(NULL, "%s, Section A.2 failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
					return (mi_false);
				}

				//copy remain data to destination ptr
				for (i = 0; i < vSizeRemain; i++) {
					*p = data[i];
					p++;
				}
			//	wt_bl_debug_print(NULL, "%s, Section A.2 read data[0~3]:", __FUNCTION__);
				//rs_print_data_array(destination, 4);
				//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
				//rs_print_data_array((destination + size - 4), 4);
				return (mi_true);
			}
		}
		// A.3 Aligned adrress, but size < NOR_PAGE_SIZE
		//copy data directly to destination ptr
		for (i = 0; i < vSizeRemain; i++) {
			*p = data[i];
			p++;
		}
		//wt_bl_debug_print(NULL, "%s, Section A.3 read data[0~3]:", __FUNCTION__);
		//rs_print_data_array(destination, 4);
		//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
		//rs_print_data_array((destination + size - 4), 4);
		return (mi_true);
	}

	// Section B: size > NOR_PAGE_SIZE
//	wt_bl_debug_print(NULL, "%s, run Section B\n\r", __FUNCTION__);
	p = destination;//pointer to destination ptr
	// According to No-aligned Address, read some data align to page
	if (vAlignOffset != 0) {
		// first, read one page first
		if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s, Setion B.1 failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}
		// copy one page to data ptr
		for (i = vAlignOffset; i < NOR_PAGE_SIZE; i++) {
			*p = data[i];
			p++;
		}
		vSizeCount = ((size - (NOR_PAGE_SIZE - vAlignOffset)) / NOR_PAGE_SIZE);
		vSizeRemain = ((size - (NOR_PAGE_SIZE - vAlignOffset)) % NOR_PAGE_SIZE);
		// next page
		vPage++;
		// check page
		if (vPage > (blk_page_count - 1)) {
			vBlock++;
			vPage = 0;
			// check block count
		}
		// init data array		
		memset(data, 0x00, NOR_PAGE_SIZE);
	}

//	wt_bl_debug_print(NULL, "%s, after resize, vBlock[%d] vPage[%d], vSizeCount[%d], vSizeCount[%d]\n\r", __FUNCTION__, vBlock, vPage, vSizeCount, vSizeRemain);
	// Now the address && data && destination ptr is aligned to NOR_PAGE_SIZE
	// Section B.1:  size > NOR_PAGE_SIZE, No-aligned Address, between two pages + some remain size(<NOR_PAGE_SIZE)
	if (vSizeCount == 0) {
		// read one page first
		if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s, Setion B.1 failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}
		// copy read-data to data ptr
		for ( i = 0; i < vSizeRemain; i++) {
			*p = data[i];
			p++;
		}
		//wt_bl_debug_print(NULL, "%s, Section B.1 read data[0~3]:", __FUNCTION__);
		//rs_print_data_array(destination, 4);
		//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
		//rs_print_data_array((destination + size - 4), 4);	
		return (mi_true);
	}

//	wt_bl_debug_print(NULL, "%s, circle before, vSizeCount[%d]\n\r", __FUNCTION__, vSizeCount);
	// Now cycle read, address && data && destination ptr is aligned to NOR_PAGE_SIZE
	// Section B.2 and B.3 and B.4, n * NOR_PAGE_SIZE, perhaps remain data size
	for (j = 0 ; j < vSizeCount; j++) {
//		wt_bl_debug_print(NULL, "%s, circling, vBlock[%d] vPage[%d], vSizeCount[%d], vSizeRemain[%d]\n\r", __FUNCTION__, vBlock, vPage, vSizeCount, vSizeRemain);
		// read data
		if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s, Setion B.2 || B.3 failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}
		//wt_bl_debug_print(NULL, ", vpage = %d,data[%d~%d]:", vPage,0,3);
		//rs_print_data_array(data, 4);
		// copy one page data to destination ptr
		for (i = 0; i < NOR_PAGE_SIZE; i++) {
			*p = data[i];
			p++;
		}
		
		// next page
		vPage++;
//		wt_bl_debug_print(NULL, "%s, after resize ???, get vPage[%d]\n\r", __FUNCTION__, vPage);
		// check page
		if (vPage > (blk_page_count - 1)) {
			vBlock++;
			vPage = 0;
			// check block count
		}
		// init data array		
		memset(data, 0x00, NOR_PAGE_SIZE);
	}

//	wt_bl_debug_print(NULL, "%s, after resize circle, vSizeCount[%d], vBlock[%d], vPage[%d], vSizeRemain[%d]\n\r", __FUNCTION__, vSizeCount, vBlock, vPage, vSizeRemain);
	// Section B.2 and B.3 and B.4
	// Section B.2: NO-aligned address, some pages + some remain size(<NOR_PAGE_SIZE)
	// Section B.3: Aligned address, some pages + some remain size(<NOR_PAGE_SIZE)
	// Section B.4: Aligned address, some pages + no remain size
	if (vSizeRemain != 0) {
//		wt_bl_debug_print(NULL, "%s, after resize 111, block[%d] page[%d], size count[%d], size remain[%d]\n\r", __FUNCTION__, vBlock, vPage, vSizeCount, vSizeRemain);
		// read data
		if (g_ftlFuncTbl->FTL_ReadPage(vBlock, vPage, (uint32_t *)data, NULL) != FTL_SUCCESS) {
			wt_bl_debug_print(NULL, "%s, Setion B.2 || B.3 failed causion:read error at block[%d] page[%d], quit\n\r", __FUNCTION__, vBlock, vPage);
			return (mi_false);
		}
		// copy remain data to destination ptr
		for (i = 0; i < vSizeRemain; i++) {
			*p = data[i];
			p++;
		}
		//wt_bl_debug_print(NULL, "%s, Section B.2 || B.3 read data[0~3]:", __FUNCTION__);
		//rs_print_data_array(destination, 4);
		//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
		//rs_print_data_array((destination + size - 4), 4);
		return (mi_true);
	}
	//wt_bl_debug_print(NULL, "%s, Section B.4 read data[0~3]:", __FUNCTION__);
	//rs_print_data_array(destination, 4);
	//wt_bl_debug_print(NULL, ", data[%d~%d]:", (size - 4), (size - 1));
	//rs_print_data_array((destination + size - 4), 4);
	return(mi_true);
}


unsigned int mi_bl_read_flash(unsigned int addr,unsigned char* data_ptr, unsigned int len)
{
	mi_bool ret=0;
	ret = rs_ua_flash_read(data_ptr, addr, len);
	
	if(ret == mi_true)
		return 0;       //success
	else
		return 1;		//fail
}


int mi_bl_earse_flash(uint32_t start_address, uint32_t size)
{
	uint32_t vBlock;
	uint32_t vPage;
	uint32_t vAddress = ((uint32_t)start_address);//note, maybe error


	
	// address to Block/Page
	if (!mi_ua_addrToBlockPage(vAddress, &vBlock, &vPage))
	{
		wt_bl_debug_print(NULL, "%s, failed causion:transfer address to block/page failed, quit\n\r", __func__);
		return (mi_false);
	}
	
	wt_bl_debug_print(NULL, "%s, vBlock is %d, vPage is %d\n\r", __func__, vBlock, vPage);
	// check erase block - page
	if (vPage != 0)
	{
		wt_bl_debug_print(NULL, "%s, failed causion:we must erase an entire block!!!\n\r", __func__);
		return (mi_false);
	}

	// erase
	if (g_ftlFuncTbl->FTL_EraseBlock(vBlock, NULL) != FTL_SUCCESS){
		wt_bl_debug_print(NULL, "%s, failed causion:erase error at block[%d] page[%d], quit\n\r", __func__, vBlock, vPage);
		return (mi_false);
	}

	return(mi_true);
}


unsigned int mi_bl_addr_to_block(unsigned int addr)
{
   #ifdef __NOR_SUPPORT_RAW_DISK__ 
   return sm_get_block_index(addr);
   #endif
}


unsigned int mi_bl_disk_read(unsigned int address, uint8_t *destination, unsigned int size)
{
	unsigned int result;
#ifdef __NOR_SUPPORT_RAW_DISK__ 
   	result = sm_read_buffer(address, destination,size); 
	if(result == RAW_DISK_ERR_NONE)
	{		
		wt_bl_debug_print(NULL, "%s success at addr[%d], len[%d]\n\r", __FUNCTION__, address, size);
		return 0;
	}
	else
	{	
		wt_bl_debug_print(NULL, "%s error at addr[%d], len[%d], ret[%d]\n\r", __FUNCTION__, address, size, result);
		return 1;
	}
#endif

	return 1;
}




unsigned int mi_bl_disk_write(unsigned int address,
                       uint8_t *source,
                       unsigned int size)
{
	unsigned int result;
	#ifdef __NOR_SUPPORT_RAW_DISK__
	result=sm_write_buffer(address,source,size);
	if(result == RAW_DISK_ERR_NONE)
	{		
		wt_bl_debug_print(NULL, "%s success at addr[%d], len[%d]\n\r", __FUNCTION__, address, size);
		return 0;
	}
	else
	{	
		wt_bl_debug_print(NULL, "%s error at addr[%d], len[%d], ret[%d]\n\r", __FUNCTION__, address, size, result);
		return 1;
	}
	#endif	

	return 1;
}


unsigned int mi_bl_disk_earse(uint32_t addr, uint32_t size)
{
	unsigned int block_idx;

	block_idx = mi_bl_addr_to_block(addr);
#ifdef __NOR_SUPPORT_RAW_DISK__
 	sm_erase_block(block_idx);
#endif
	return 1;
}

//查看当前的升级状态
int mi_iot_get_fota_status(int *status)
{
	int ret;
	char status_buf[4];
	int test;

	if(NULL == status)
	{
		return -1;
	}
	
	ret = mi_bl_read_flash(BL_FLASH_OTA_UPDATE_STATUS_ADDR, status_buf, BL_FLASH_OTA_UPDATE_STATUS_LEN);
	wt_bl_debug_print(NULL, "[WT_FOTA] read status result: %d\r\n", ret);

	memcpy(status, status_buf, BL_FLASH_OTA_UPDATE_STATUS_LEN);
	memcpy(&test, status_buf, BL_FLASH_OTA_UPDATE_STATUS_LEN);	
	wt_bl_debug_print(NULL, "[WT_FOTA] ota status: %d\r\n", test);
	
	return ret;
}

//更新升级状态
void mi_iot_set_fota_status(int up_status)
{
	int ret;
	char status_buf[4] = {0};

	memset(earse_buf, 0x00, BLOCK_SIZE);
	ret = mi_bl_read_flash(BL_FLASH_OTA_UPDATE_STATUS_ADDR&F_SECTOR_MASK, earse_buf, BLOCK_SIZE);
	wt_bl_debug_print(NULL, "mi_iot_set_fota_status read: %d\r\n", ret);
	mi_bl_earse_flash(BL_FLASH_OTA_UPDATE_STATUS_ADDR&F_SECTOR_MASK, BLOCK_SIZE);
	memcpy(earse_buf + BL_FLASH_OTA_UPDATE_STATUS_ADDR - (BL_FLASH_OTA_UPDATE_STATUS_ADDR&F_SECTOR_MASK), &up_status, BL_FLASH_OTA_UPDATE_STATUS_LEN);
	ret = mi_bl_write_flash(BL_FLASH_OTA_UPDATE_STATUS_ADDR&F_SECTOR_MASK, earse_buf, BLOCK_SIZE);
	wt_bl_debug_print(NULL, "mi_iot_set_fota_status write: %d\r\n", ret);

	//free(earse_buf);
}

//删除固件
int mi_iot_erase_firmware(void)
{
	return 0;
}

//删除备份
int mi_iot_erase_backup(void)
{
	return 0;
}

void mi_iot_execute(void)
{
	int status = -1;
	int ret;
	mi_updateState up_status;
	
	wt_bl_debug_print(NULL, "[WT_FOTA] mi_iot_execute()\r\n");
	flash_api.erase = mi_bl_earse_flash;
	flash_api.read = mi_bl_read_flash;
	flash_api.write = mi_bl_write_flash;
	
	ret = mi_iot_get_fota_status((int *)(&up_status));
	if(ret != 0)
	{
		//获取状态错误退出
		wt_bl_debug_print(NULL, "[WT_FOTA] get fota status error\r\n");
		return;
	}	
	wt_bl_debug_print(NULL, "[WT_FOTA] fota status %d\r\n", up_status);
	
	if(UPSTATE_UPDATE == up_status){
		//已执行升级动作、中间发生掉电或其他动作导致升级未完成

	}
	
	wt_bl_debug_print(NULL, "==ready to call function wt_iot_patch\n\r");
	switch(up_status){
		case UPSTATE_IDLE:
			break;
		case UPSTATE_CHECK:
			mi_iot_set_fota_status(UPSTATE_UPDATE);
			up_status = UPSTATE_UPDATE;
			status = firmware_patch(&flash_api, &mi_flash_info);
			break;
		case UPSTATE_UPDATE:
			status = firmware_patch(&flash_api, &mi_flash_info);
			break;
		case UPSTATE_UPDATEED:
			break;
		default:
			break;
	}
	
	//如果升级成功
	if((0 == status) || (up_status == UPSTATE_UPDATEED)){
		//wt_bl_ShowUpateSuccess();
		mi_iot_set_fota_status(UPSTATE_UPDATEED);
		//删除固件
		mi_iot_erase_firmware();
		//删除备份
		mi_iot_erase_backup();
		mi_iot_set_fota_status(UPSTATE_IDLE);
	}
	return;
}


void mi_iot_update_moudle(void)
{
	int i, percent; 
	
	mi_iot_get_working_buffer();
	mi_flash_info.backup_addr = BL_FLASH_OTA_BACKUP_ADDR;
	mi_flash_info.backup_size = BL_FLASH_OTA_BACKUP_SIZE;
	mi_flash_info.download_addr = BL_FLASH_OTA_UPDATE_ADDR;
	mi_flash_info.download_size = BL_FLASH_OTA_UPDATE_SIZE;
	mi_flash_info.firmware_addr = 0x018000;
	mi_flash_info.firmware_size = 0x5F8000;
	mi_flash_info.sector_size = BLOCK_SIZE;
	
	WacthDogDisable();
	CacheInit(); 
	wt_bl_debug_print(NULL, "[WT_FOTA] ps addr is 0x%x, ps size is 0x%x\r\n", mi_flash_info.firmware_addr, mi_flash_info.firmware_size);
	wt_bl_debug_print(NULL, "[WT_FOTA] backup addr is 0x%x, backup size is 0x%x\r\n", mi_flash_info.backup_addr, mi_flash_info.backup_size);
	wt_bl_debug_print(NULL, "[WT_FOTA] download addr is 0x%x, download size is 0x%x\r\n", mi_flash_info.download_addr, mi_flash_info.download_size);
	mi_iot_execute();	
	CacheDeinit();
	WacthDogRestart();
	return;
}

