#include <stdlib.h>
#include <stdint.h>
//#include <stdio.h>
#include <string.h>

#include "LZMA_decoder.h"
#include "arch_interface.h"
#include "sm_fota.h"


#ifdef RUN_ON_PC
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#endif




/*
 * 估算RAM总的使用量:
 * 静态数组为16K+16K+4K+4K+16K(ant_plan_info)=56K
 * 函数内部执行栈调用：4K+4K+OTHERS，16K应该是比较保险的
 * 总共需要RAM大约在72K左右.由于patch操作一般是在bootloader里面实现，所以不会占用应用层RAM
 * ant_plan_info结构体里面的数组可根据实际情况调整，可以总体优化到64K以内
 */

static ant_plan_t ant_plan_info;

extern uint32_t Image$$EXT_BL_IOT_WT_FOTA_WORKING_BUF$$ZI$$Base;

uint8_t * lzma_buf_in = 0;
uint8_t * lzma_buf_out = 0;
uint8_t * back_up_buf = 0;
uint8_t * diff_buf = 0;
uint8_t * decode_buf = 0;
uint8_t * earse_buf = 0;



static uint8_t lzma_buf_0[16384]; /* 15980 */
static uint8_t lzma_buf_1[16384]; /* 16384 */
//static uint8_t lzma_buf_in[IN_BUF_SIZE];  	/* 4096 */
//static uint8_t lzma_buf_out[OUT_BUF_SIZE];  /* 4096 */
//static uint8_t back_up_buf[F_SECTORE_SIZE];  /* 4096 */
//static uint8_t diff_buf[F_SECTORE_SIZE];  /* 4096 */
//static uint8_t earse_buf[32*1024];  /* 4096 */
//static uint8_t decode_buf[F_SECTORE_SIZE];  /* 4096 */
static uint32_t totol_Size = 0;


//2. flash interface
int arch_write_flash(uint32_t start_address, uint8_t *buf, uint32_t length)
{
 	unsigned int ret = 0;
	uint32_t vAlignOffset = 0;



	if(0 == length)
	{
		wt_bl_debug_print(NULL, "write flash length is 0...\r\n");
		return 0;
	}
	
	wt_bl_debug_print(NULL, "write flash addr is %8x, len is %d.\r\n",start_address,length);

	if(start_address & (BLOCK_SIZE - 1))
	{
		
		vAlignOffset = start_address & (BLOCK_SIZE - 1);
		start_address = (start_address - vAlignOffset);
		memset(earse_buf, 0xFF, BLOCK_SIZE);
		ret = mi_bl_read_flash(start_address, earse_buf, BLOCK_SIZE);
		if(ret != 0)
			return 1;
		
		ret = mi_bl_earse_flash(start_address, BLOCK_SIZE);		
		if(ret != mi_true)
			return 1;
		
		memcpy(earse_buf + vAlignOffset, buf, length);
		ret = mi_bl_write_flash(start_address, earse_buf, BLOCK_SIZE);
	}
	else
	{
 		ret = (unsigned int)mi_bl_write_flash(start_address, buf, length); //NOTE: src must be from ram, not in nor
	}
	
	if(ret == mi_true)
		return 0;       //success
	else
		return 1;		//fail
		
}


int arch_read_flash(uint32_t start_address, uint8_t *buffer, uint32_t length)
{
	unsigned int ret;
	
	wt_bl_debug_print(NULL, "read flash addr is %8x, len is %d.\r\n",start_address,length);
	
	ret = (unsigned int)mi_bl_read_flash(start_address, buffer, length);
		
	return ret;
}


//注意,该函数不可跨区操作,当start_address + size跨了两个block时会出错切记
int arch_erase_flash(uint32_t start_address, uint32_t size)
{
	wt_bl_debug_print(NULL, "arch_erase_flash %x\r\n",start_address);
	
	mi_bl_earse_flash(start_address, size);

	return 0;
}

//2. flash interface
int arch_write_disk(uint32_t start_address, uint8_t *buf, uint32_t length)
{
 	unsigned int ret = 0;

	if(0 == length)
	{
		wt_bl_debug_print(NULL, "write disk length is 0...\r\n");
		return 0;
	}
	wt_bl_debug_print(NULL, "arch_write_disk addr is %8x, len is %d.\r\n",start_address,length);
 	ret = mi_bl_disk_write(start_address, buf, length); //NOTE: src must be from ram, not in nor
	
	return ret;
}


int arch_read_disk(uint32_t start_address, uint8_t *buffer, uint32_t length)
{
	unsigned int ret;
	
	wt_bl_debug_print(NULL, "arch_read_disk addr is %8x, len is %d.\r\n",start_address,length);
	
	ret = mi_bl_disk_read(start_address, buffer, length);
		
	return ret;
}


//注意,该函数不可跨区操作,当start_address + size跨了两个block时会出错切记
int arch_erase_disk(uint32_t start_address, uint32_t size)
{
	wt_bl_debug_print(NULL, "arch_erase_disk %x\r\n",start_address);
	
	mi_bl_disk_earse(start_address, size);

	return 0;
}


void mi_iot_get_working_buffer()
{
	lzma_buf_in = (uint8_t *)(&Image$$EXT_BL_IOT_WT_FOTA_WORKING_BUF$$ZI$$Base);
	lzma_buf_out = lzma_buf_in+IN_BUF_SIZE;
	back_up_buf = lzma_buf_out+OUT_BUF_SIZE;
	diff_buf = back_up_buf+F_SECTORE_SIZE;
	decode_buf = diff_buf+F_SECTORE_SIZE;
	earse_buf = decode_buf + F_SECTORE_SIZE;
}

//实现本功能，必须包含以下flash分区
/*
 * firmware flash: 应用固件分区，存放一个完整的运行固件
 * download flash: 下载地址，用来下载一个差分固件，这个分区的大小，大约为firmware flash大小的10%~20%，越大表示能支持的固件差异化也越大
 *　backup flash  : 差分过程中备份用的flash,分为以下三部分：
 *　		backup sea			: 备份firmware flash的旧固件，大约10% ~20% firmware flash大小，同样越大表示能支持的固件差异化也越大
 *		backup sector status: 状态管理　	4K
 *		topatch block status: 状态管理	4K
 *
 * 对flash的要求：最小sector需支持4K,如果不支持，需要对程序进行调整
 * 上述所有的flash 分区必须对齐到sector
 */


flash_handle_t flash_handle = {arch_write_flash, arch_read_flash, arch_erase_flash};


/* LZMA porting ( decompress a bin always use 4 buffer ) */
void *_bl_alloc(void *p, uint32_t size)
{
	static int lzma_alloc_count = 0;
	wt_bl_debug_print(NULL, "_bl_alloc size = %d \r\n", size);
	wt_bl_debug_print(NULL, "_bl_alloc lzma_alloc_count = %d \r\n", lzma_alloc_count);
    switch (lzma_alloc_count) {
        case 0:
            lzma_alloc_count++;
            return &lzma_buf_0;
        case 1:
        	lzma_alloc_count = 0;
            return &lzma_buf_1;
        default:
            wt_bl_debug_print(NULL, "_bl_malloc error.\r\n");
            return NULL;
    }
}
void _bl_free(void *p, void *address)
{
	wt_bl_debug_print(NULL, "_bl_free  \r\n");
}
lzma_alloc_t lzma_alloc = { _bl_alloc, _bl_free };


//little endian used in bidiff
static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

static int32_t offtin_s32(uint8_t *buf)
{
	int32_t y;

	y = buf[3]&0x7F;
	y=y<<8; y+=buf[2];
	y=y<<8; y+=buf[1];
	y=y<<8; y+=buf[0];

	if(buf[3]&0x80) y=-y;
	return y;
}

static int set_all_config(ant_plan_t *pant, flash_handle_t *f_handle, flash_des_t *f_info)
{
	int ret=0;

	pant->phandle		= f_handle;

	pant->f_fw_addr 	= f_info->firmware_addr;	//must align to F_SECTORE_SIZE
	pant->f_dw_addr		= f_info->download_addr;	//must align to F_SECTORE_SIZE
	pant->f_max_fw_size	= f_info->firmware_size;
	pant->f_sector_size	= f_info->sector_size;

	pant->b_total		= 0;
	pant->b_max			= MAX_TOPATCH_BLOCK_NUM;
	memset(pant->b_diff_ctrl, 0, sizeof(pant->b_diff_ctrl));

	pant->p_newpos		= 0;
	pant->p_oldpos		= 0;

	pant->bk_sea_addr	= f_info->backup_addr;
	pant->bk_max_size 	= f_info->backup_size - f_info->sector_size;
	pant->bk_magic_addr = f_info->backup_addr+f_info->backup_size-f_info->sector_size;
	pant->bk_status_addr= pant->bk_magic_addr+BACKUP_MAGIC_SIZE;
	pant->bk_max_sector	= pant->bk_max_size / f_info->sector_size;
	pant->bk_map_used	= 0;
	pant->bk_used_oldest= 0;
	memset(pant->bk_r_status, 0xff, sizeof(pant->bk_r_status));
	memset(pant->bk_map, 0, sizeof(pant->bk_map));

	// XXX XXX XXX bk_status_addr and bk_magic_addr erase in restore_last_patch()

    /*
     * 								***IMPORTANT***
     * f_fw_addr/f_dw_addr/b_status_addr/bk_status_addr MUST ALIGN TO F_SECTORE_SIZE
     */
	if((pant->bk_magic_addr % pant->f_sector_size != 0) ||
		(pant->bk_sea_addr % pant->f_sector_size != 0)	||
		(pant->f_fw_addr % pant->f_sector_size != 0) ||
		(pant->f_dw_addr % pant->f_sector_size != 0) ||
		(pant->f_dw_addr % pant->f_sector_size != 0) ||
		(pant->f_sector_size < 0x1000)){
		wt_bl_debug_print(NULL, "***init config error***.\r\n");
		return -1;
	}

	wt_bl_debug_print(NULL, "init all config.\r\n");
	wt_bl_debug_print(NULL, "bk_magic_addr: %x\r\n", pant->bk_magic_addr);

	return ret;
}


/*
 * lzma decode :
 * 就像从ram里面读取解码数据一样方便
 * 返回值：０：正常；其他：错误
 */
static int read_decode_by_length(ant_plan_t *ant_plan, decode_f_man *dec_data , uint8_t *p_de_buf, uint32_t de_len)
{
	int ret;
	uint32_t buf_pos=0;
	Byte *inBuf, *outBuf;
	SizeT inProcessed;
	SizeT outProcessed;
	ELzmaStatus status;
	ELzmaFinishMode finishMode;
	int32_t percent = 0;

    /* variable for read and write flash */
    inBuf = dec_data->inbuf;
    outBuf = dec_data->outbuf;

    if(dec_data->unpackSize == (uint32_t)(int32_t)-1){
    	return -1;
    }

    for (;;)
    {
		
        if (dec_data->in_pos == dec_data->in_size) {

        	//check is reach flash end addr?
        	if(dec_data->src_curr > dec_data->src_end){
        		wt_bl_debug_print(NULL, "read decode error: flash is over.\r\n");
        		return -2;
        	}

			//最后一个block数据不足32K, 写成32K的会over read
        	dec_data->in_size = MIN(dec_data->src_end-dec_data->src_curr, dec_data->inbuf_size);
            wt_bl_debug_print(NULL, "LZMA_read_data, addr = 0x%x ,size = 0x%x\n\r", (uint32_t)dec_data->src_curr, dec_data->in_size);

            ret = arch_read_flash((uint32_t)dec_data->src_curr, inBuf, dec_data->in_size);
            if (ret != 0) {
                wt_bl_debug_print(NULL, "read data fail \n");
                return -3;
            }
            dec_data->src_curr += dec_data->in_size;
            dec_data->in_pos = 0;
        }

        inProcessed = dec_data->in_size - dec_data->in_pos;

        outProcessed = MIN(dec_data->outbuf_size , de_len);
        finishMode = LZMA_FINISH_ANY;
        if (outProcessed > dec_data->unpackSize){
            outProcessed = (SizeT)dec_data->unpackSize;
            finishMode = LZMA_FINISH_END;
        }

        //LZMA_LOG("LZAM_decode_begin: inPos = %d \n", inPos);
        ret = LzmaDec_sm_DecodeToBuf(&dec_data->state, outBuf, &outProcessed,
          inBuf + dec_data->in_pos, &inProcessed, finishMode, &status);
        if (ret != SZ_OK){
        	wt_bl_debug_print(NULL, "lzma decode err: %d, done is %d.\r\n",ret,ant_plan->p_newpos);
        	return -4;
        }

		
        memcpy(p_de_buf+buf_pos, outBuf, outProcessed);
		
        de_len  -= outProcessed;
        buf_pos += outProcessed;
        dec_data->in_pos += inProcessed;
        dec_data->unpackSize -= outProcessed;
		
		
        if (dec_data->unpackSize == 0 || de_len == 0)  {
			
			percent = (int32_t)((float)(totol_Size-dec_data->unpackSize)/totol_Size*100);
			wt_bl_progress(percent);
        	wt_bl_debug_print(NULL, "Decode OK: Remain unpacksize = %d, Done =%d\r\n",dec_data->unpackSize, buf_pos);
            return 0;
        }

        if (inProcessed == 0 && outProcessed == 0) {
            if (status != LZMA_STATUS_FINISHED_WITH_MARK) {
                return -5;//return SZ_ERROR_DATA;
            }
            return ret;
        }
    }

    //should't run here
    return 0;
}



int get_topatch_chain( decode_f_man *dec_data, ant_plan_t *ant_plan)
{
	int ret=0;
	uint32_t magic;
	uint32_t topatch_block_len;
	uint32_t i;
	uint8_t head[8];
#ifdef ENABLE_DEBUG
	int32_t block_start=0;
#endif

    //LZMA_LOG("LZAM_decode_begin: inPos = %d \n", inPos);
	//decode first 8 bytes, 4byte magic, 4bytes block chain length	
    ret = read_decode_by_length(ant_plan, dec_data, head, 8);
    if(ret != 0){
    	wt_bl_debug_print(NULL, "GTC1:decode err:%d.\r\n",-ret);
    	return -1;
    }

    magic = *(uint32_t*)head;
    if(memcmp(&magic, "ANUI", sizeof("ANUI")-1) != 0){
    	wt_bl_debug_print(NULL, "GTC:magic err:%s.\r\n",head);//magic is not string
    	return -2;
    }

    //TODO,must check version correct here

    wt_bl_debug_print(NULL, "magic is %s.\r\n",head);

    topatch_block_len =0;
    for(i=0; i<4; i++)
    	topatch_block_len += (uint32_t)head[4+i] << (i*8);	//TODO,Must be big endian

    ant_plan->b_total = topatch_block_len/sizeof(struct diff_ctrl_str);
    if((topatch_block_len % sizeof(struct diff_ctrl_str)) || (ant_plan->b_total > ant_plan->b_max )){
    	wt_bl_debug_print(NULL, "chain block num error :%d. \r\n",ant_plan->b_total);
    	return -3;
    }
    wt_bl_debug_print(NULL, "GTC:chain len:%d .\r\n",topatch_block_len);

    ret = read_decode_by_length(ant_plan, dec_data, (uint8_t *)ant_plan->b_diff_ctrl, topatch_block_len);
    if(ret != 0){
    	wt_bl_debug_print(NULL, "GTC2:decode err:%d.\r\n",-ret);
    	return -4;
    }

    for(i=0; i<ant_plan->b_total; i++){
    	ant_plan->b_diff_ctrl[i].diff = offtin_s32((uint8_t *)&ant_plan->b_diff_ctrl[i].diff);
    	ant_plan->b_diff_ctrl[i].adjust = offtin_s32((uint8_t *)&ant_plan->b_diff_ctrl[i].adjust);

    	/*****just for test****/
#ifdef ENABLE_DEBUG
    	wt_bl_debug_print(NULL, "block chain,SN=%d, start=%d,diff len=%d.\r\n",i,block_start,ant_plan->b_diff_ctrl[i].diff);
    	block_start += (ant_plan->b_diff_ctrl[i].diff + ant_plan->b_diff_ctrl[i].adjust);
#endif
    }

    return ret;
}


//要求：尽量对back sea sector均衡化处理
//TODO,backup 应该以当前patch完成的newsize来检测是否失效
//注：本函数计算量比较大
static int check_and_recycle_old_backup(ant_plan_t * ant_plan)
{
	int ret=0;
	uint32_t i,j;
	int32_t block_start=0;
	int32_t block_end=0;
	uint32_t sector_num;
	uint32_t bk_sector_addr;
	uint32_t write;
	uint32_t flag=0;
	uint32_t curr_block_start=0;

	wt_bl_debug_print(NULL, "check and recycle.\r\n");

	//check whether there is valid sector
	if(ant_plan->bk_map_used < ant_plan->bk_max_sector){
		wt_bl_debug_print(NULL, "back sea has valid sector, bk_map_used is %d.\r\n",ant_plan->bk_map_used);
		return 0;
	}

	//no valid sector, we need to recycle.

	//get current block start_addr & end_addr
	for(j=0; j<ant_plan->b_curr_block; j++){
		curr_block_start += ant_plan->b_diff_ctrl[j].diff;
		curr_block_start += ant_plan->b_diff_ctrl[j].adjust;
	}

//	wt_bl_debug_print(NULL, "back used oldest is %d.\r\n",ant_plan->bk_used_oldest);
	for(i=ant_plan->bk_used_oldest;i<F_BK_POOL_NUM; i++){

		if(ant_plan->bk_r_status[i].block_status == BACK_FLASH_STATUS_S){//仍然有效

			bk_sector_addr = i*ant_plan->f_sector_size;

			block_start = curr_block_start;

			//以下是判断是否失效逻辑，todo, block start and block end ?
			for(j=ant_plan->b_curr_block; j< ant_plan->b_total; j++){
				block_end = block_start + ant_plan->b_diff_ctrl[j].diff ;

				if(j == ant_plan->b_curr_block){
					//应该检查一下p_oldpos是否在block start & block end之间
					if(((bk_sector_addr+ant_plan->f_sector_size) >= ant_plan->p_oldpos) && (bk_sector_addr <= block_end)){
						wt_bl_debug_print(NULL, "^^^^HIT cblock=%d, start=0x%4x, end=0x%4x,bk start=0x%4x \r\n",j,block_start,block_end,bk_sector_addr);
						break;
					}
				}
				else{
					if(((bk_sector_addr+ant_plan->f_sector_size) >= block_start) && (bk_sector_addr <= block_end)){
						wt_bl_debug_print(NULL, "^^^^HIT block=%d, start=0x%4x, end=0x%4x,bk start=0x%4x \r\n",j,block_start,block_end,bk_sector_addr);
						break;
					}
				}

				block_start += ant_plan->b_diff_ctrl[j].diff;
				block_start += ant_plan->b_diff_ctrl[j].adjust;
			}
			//失效判断完成

			if(j == ant_plan->b_total){//说明失效

				wt_bl_debug_print(NULL, "-----The back flash addr:%4x is unvalid, bk_map_used is %d,i is %d.\r\n",bk_sector_addr,ant_plan->bk_map_used,i);	//TODO
				sector_num = ant_plan->bk_r_status[i].sea_block_num;
				ant_plan->bk_r_status[i].block_status = BACK_FLASH_STATUS_C;

				ant_plan->bk_map[sector_num] = BACKUP_FREE;
				ant_plan->bk_map_used--;
				flag = 1;

				//状态要同步到flash里面;
				write = ant_plan->bk_status_addr + sizeof(back_status_t)*i;
				ret = arch_write_flash(write, (uint8_t *)&ant_plan->bk_r_status[i].block_status, sizeof(uint16_t));
				if(ret != 0)return -1;

				break;//exit "for(i=ant_plan->bk_used_oldest; i<ant_plan->bk_max_sector; i++)"
			}
		}
	}

	for(i=ant_plan->bk_used_oldest;i<F_BK_POOL_NUM; i++){

		if(ant_plan->bk_r_status[i].block_status == BACK_FLASH_STATUS_C){//仍然有效
			ant_plan->bk_used_oldest = i;//bk_used_oldest只是为了加速搜索，特别是在patch后期
		}
		else break;
	}

	if(flag == 1)
		return 0;
	else{
		wt_bl_debug_print(NULL, "recycle error: no unvalid sector to release,i is%d, max is %d.\r\n",i,ant_plan->bk_max_sector);
		return -2;
	}

	return ret;
}




static int backup_old_sector(ant_plan_t * ant_plan, uint32_t back_addr)
{
	uint32_t ret = 0;
	uint32_t i=0;
	int32_t oldpos=0;
	uint32_t temp;
	uint32_t magic;
	uint32_t sector;
	uint32_t write;
	uint16_t j=0;

	if(back_addr % ant_plan->f_sector_size){
		return -1;
	}

	wt_bl_debug_print(NULL, "\r\n***check if need backup,addr =0x%x,block total =%d***.\r\n",back_addr,ant_plan->b_total);

	sector = back_addr/ant_plan->f_sector_size;
	if(ant_plan->bk_r_status[sector].block_status == BACK_FLASH_STATUS_S){
		wt_bl_debug_print(NULL, "flash had back, addr is %4x.\r\n", back_addr);
		return 0;
	}

	//recycle here:
	ret = check_and_recycle_old_backup(ant_plan);
	if(ret != 0){
		return -11;
	}

	for(i=0; i< ant_plan->b_total; i++){

		temp = oldpos + ant_plan->b_diff_ctrl[i].diff;

		if((back_addr + ant_plan->f_sector_size >= oldpos) && (back_addr <= temp) ){

			//找一块可用的back sea sector
			for(j=0; j< ant_plan->bk_max_sector ;j++){
				if(ant_plan->bk_map[j] == BACKUP_FREE)
					break;
			}

			if(j != ant_plan->bk_max_sector){

				wt_bl_debug_print(NULL, "***need backup, addr is %8x, bk_map_used is %d***\r\n",back_addr,ant_plan->bk_map_used);

				ant_plan->bk_map[j] = BACKUP_BUSY;

				ret = arch_read_flash(ant_plan->f_fw_addr+back_addr, back_up_buf, ant_plan->f_sector_size);
				if(ret != 0) return -2;
				write = ant_plan->bk_sea_addr + j*ant_plan->f_sector_size;
				ret = arch_erase_flash(write, ant_plan->f_sector_size);
				if(ret != 0) return -3;
				ret = arch_write_flash(write, back_up_buf, ant_plan->f_sector_size);
				if(ret != 0) return -4;

				//更新对应的状态表，RAM & ROM
				ant_plan->bk_r_status[sector].sea_block_num= j;
				ant_plan->bk_r_status[sector].block_status = BACK_FLASH_STATUS_S;

				write = ant_plan->bk_status_addr+sizeof(back_status_t)*sector;
				ret = arch_write_flash(write, (uint8_t*)&ant_plan->bk_r_status[sector], sizeof(back_status_t));
				if(ret != 0) return -5;

				ant_plan->bk_map_used++;

			}else{
				wt_bl_debug_print(NULL, "BACK error: no more flash to back.\r\n");
			}
			return 0;
		}
		oldpos += ant_plan->b_diff_ctrl[i].diff;
		oldpos += ant_plan->b_diff_ctrl[i].adjust;	//adjust is int, may < 0
	}

	return ret;
}


static int patch_diff(ant_plan_t * ant_plan, uint8_t *pbuf, uint32_t diff_len)
{
	int ret=0;
	int i,j;
	int32_t new_pos;
	int32_t old_pos;
	uint32_t sector;
	uint32_t op_len=0;
	uint32_t flash_addr;
	uint32_t diff_index=0;
	uint32_t repeat;
	uint32_t write;

	memset(diff_buf, 0xFF, F_SECTORE_SIZE);
	new_pos = ant_plan->p_newpos;
	old_pos = ant_plan->p_oldpos;

	//检查diff_len的长度不大于HAL_FLASH_SECTOR_SIZE．并且，new+diff_len不能跨flash sector
	if((diff_len > ant_plan->f_sector_size) || (F_ALIGN_SECTOR(new_pos) != F_ALIGN_SECTOR(new_pos+diff_len-1))){
		wt_bl_debug_print(NULL, "patch diff: error diff_len :%d.\r\n",diff_len);
		return -1;
	}

	//如果diff的old地址覆盖两个sector，则需要拆分来处理，因为有可能前一个sector已经备份，后一个sector还没有备份
	//这两种情况，读取old数据的来源不同
	if(F_ALIGN_SECTOR(old_pos) != F_ALIGN_SECTOR(old_pos+diff_len)){
		repeat = 2;
	}else
		repeat = 1;

	//跨区操作，第一区
	for(i=0; i<repeat; i++){
		if((repeat == 2) && (i==0))
			op_len = ant_plan->f_sector_size - (old_pos % ant_plan->f_sector_size);
		else
			op_len = diff_len - op_len;

		sector = old_pos/ant_plan->f_sector_size;
		if(ant_plan->bk_r_status[sector].block_status == BACK_FLASH_STATUS_S){
			flash_addr = ant_plan->bk_sea_addr+ \
					ant_plan->f_sector_size*ant_plan->bk_r_status[sector].sea_block_num+ \
					(old_pos % ant_plan->f_sector_size);
		}
		else if(ant_plan->bk_r_status[sector].block_status == BACK_FLASH_STATUS_U){
			flash_addr = ant_plan->f_fw_addr + old_pos; //TODO，需判断是否已经被覆盖
		}
		else{
			//return OK?

			return 1;//TODO
		}
		ret = arch_read_flash(flash_addr, diff_buf, op_len);
		if(ret != 0)return -2;

		//diff patch
		for(j=0; j<op_len; j++,diff_index++)
			*(pbuf+diff_index) += diff_buf[j];
		old_pos += op_len;
	}

	//将新patch写入到new_pos
	write = ant_plan->f_fw_addr + new_pos;
	if(0 == (write % ant_plan->f_sector_size)){
		ret = arch_erase_flash(write, ant_plan->f_sector_size);
		if(ret != 0) return -3;
	}
	ret = arch_write_flash(write, pbuf, diff_len);
	if(ret != 0)return -4;

	return ret;
}


static int patch_extra(ant_plan_t *ant_plan, uint8_t *pbuf, uint32_t extra_len)
{
	int ret=0;
	uint32_t write;

	write = ant_plan->f_fw_addr + ant_plan->p_newpos;

	if(0 == (write % ant_plan->f_sector_size)){
		ret = arch_erase_flash(write, ant_plan->f_sector_size);
		if(ret != 0) return -1;
	}

	ret = arch_write_flash(write, pbuf, extra_len);
	return ret;
}


/*
 *
 * decode_and_patch
 * 解压和patch,先检查是否有一个未完成且正在进行的patch，如果有，则需要恢复相关的数据结构
 *
 * TODO,　有一点风险，sector diff finish -> recycle ,clean back sea sector->
 *
 */
#define PATCHING_ABORT_1		1	//>99%的概率，对于已经备份的内容，只能patch，不能重复patching
#define PATCHING_ABORT_2		2	//<1%的概率，只有当backup sea恰好没有剩余，当前backup sector完成且失效．开始下一个sector
static int decode_and_patch(decode_f_man *dec_info, ant_plan_t *ant_plan)
{
	int ret=0;
	uint32_t i;
	uint32_t magic;
	int32_t ctrl[3];
	uint32_t temp;
	int32_t remain;
	uint32_t last_flag=0;
	uint32_t en = 0;
	uint32_t last_bk_addr;
	uint8_t buf[F_SECTORE_SIZE];//4K

	//检查是否有升级正在进行中;
	ret = arch_read_flash(ant_plan->bk_magic_addr, (uint8_t*)&magic, sizeof(magic));
	if(ret != 0) return -1;

	if(magic != BACKUP_MAGIC){
		//magic != BACKUP_MAGIC说明还没有开始patch
		arch_erase_flash(ant_plan->bk_magic_addr,ant_plan->f_sector_size);
		en = PATCHING_ABORT_2;

		magic = BACKUP_MAGIC;
		ret = arch_write_flash(ant_plan->bk_magic_addr, (uint8_t*)&magic, sizeof(magic));
		if(ret != 0) return -2;

		wt_bl_debug_print(NULL, "unstarted patch... .\r\n");
	}else{
		//表明当前处于一个未完成的patch
		wt_bl_debug_print(NULL, "unfinished patching... magic is %8x.\r\n", magic);

		//要恢复两个b_magic_addr和bk_status_addr的数据到对应的状态表里面
		memset((uint8_t*)ant_plan->bk_r_status, 0xff, sizeof(ant_plan->bk_r_status));
		ret = arch_read_flash(ant_plan->bk_status_addr, (uint8_t*)ant_plan->bk_r_status, sizeof(ant_plan->bk_r_status));
		if(ret != 0) return -3;

		//get bk_map[]
		memset(ant_plan->bk_map, 0, sizeof(ant_plan->bk_map));
		for(i=0; i<F_BK_POOL_NUM; i++){

			if(ant_plan->bk_r_status[i].block_status == BACK_FLASH_STATUS_S){

				if(ant_plan->bk_r_status[i].sea_block_num >= ant_plan->bk_max_sector){
					wt_bl_debug_print(NULL, "***bk status->num error:%d.\r\n",ant_plan->bk_r_status[i].sea_block_num);
					return -4;
				}

				ant_plan->bk_map[ant_plan->bk_r_status[i].sea_block_num] = BACKUP_BUSY;
				ant_plan->bk_map_used++;
				last_bk_addr = i* ant_plan->f_sector_size;
				last_flag = 1;

			}else if(ant_plan->bk_r_status[i].block_status == BACK_FLASH_STATUS_C){
				last_bk_addr = i* ant_plan->f_sector_size;
				last_flag = 2;
			}else{
				//TODO:may power off when updating block status, so it's neccesory to illegal value
				if(ant_plan->bk_r_status[i].block_status != BACK_FLASH_STATUS_U)
					wt_bl_debug_print(NULL, "***warnning: illegal bk status=%4x.***\r\n",ant_plan->bk_r_status[i].block_status);
				break;
			}
		}
	}

	if(last_flag == 2)
		last_bk_addr += ant_plan->f_sector_size;

	//如果发现有topatch block正在处理，则需要从开始位置重新解压
	for(i=0; i<ant_plan->b_total; i++){
		
		ret = read_decode_by_length(ant_plan, dec_info, decode_buf, 24);
		if(ret != 0){
			wt_bl_debug_print(NULL, "read decode error or finish? ret is %d.\r\n", ret);
			return -5;
		}
		ctrl[0] = (int32_t)offtin((uint8_t *)&decode_buf[0]);
		ctrl[1] = (int32_t)offtin((uint8_t *)&decode_buf[8]);
		ctrl[2] = (int32_t)offtin((uint8_t *)&decode_buf[16]);
		//patch diff
		remain = (int32_t)ctrl[0];
		for(;;){

			//每次循环操作，都不能跨越flash sector，如果超过了，则需要切分处理
			if(F_ALIGN_SECTOR(ant_plan->p_newpos) != F_ALIGN_SECTOR(ant_plan->p_newpos+remain)){
				temp = ant_plan->f_sector_size - (ant_plan->p_newpos % ant_plan->f_sector_size);
			}else{
				temp = remain;
			}
						
			ret = read_decode_by_length(ant_plan, dec_info, decode_buf, temp);
			if(ret != 0){
				wt_bl_debug_print(NULL, "read decode error :-%d.\r\n",-ret);
				return -6;
			}
			
			if((en == 0)&&(F_ALIGN_SECTOR(ant_plan->p_newpos) == last_bk_addr)){
				en = PATCHING_ABORT_2;
				wt_bl_debug_print(NULL, "restore block=%d, sector=0x%x, flag =%d.\r\n",i, last_bk_addr,en);
			}

			if(en){
				ret = backup_old_sector(ant_plan, F_ALIGN_SECTOR(ant_plan->p_newpos ));
				if(ret != 0){
					wt_bl_debug_print(NULL, "DAP1:backup old failer,re=-%x.\r\n",-ret);
					return -8;
				}

				ret = patch_diff(ant_plan, decode_buf, temp);
				if(ret != 0){
					wt_bl_debug_print(NULL, "DAP: patch diff error:%d.\r\n", -ret);
					return -9;
				}
			}

			remain -= temp;
			ant_plan->p_newpos	+= temp;
			ant_plan->p_oldpos	+= temp;
			if(remain == 0)break;
		}
		
		//***patch extra***
		remain = (int32_t)ctrl[1];
		for(;;){

			if(F_ALIGN_SECTOR(ant_plan->p_newpos) != F_ALIGN_SECTOR(ant_plan->p_newpos+remain)){
				temp = ant_plan->f_sector_size - (ant_plan->p_newpos % ant_plan->f_sector_size);
			}else{
				temp = remain;
			}

			ret = read_decode_by_length(ant_plan, dec_info, decode_buf, temp);
			if(ret != 0) return -10;

			if((en == 0)&&(F_ALIGN_SECTOR(ant_plan->p_newpos) == last_bk_addr)){
				en = PATCHING_ABORT_2 ;
				wt_bl_debug_print(NULL, "restore block=%d, sector=%8x, flag =%d.\r\n",i, last_bk_addr,en);
			}

			if(en){
				ret = backup_old_sector(ant_plan, F_ALIGN_SECTOR(ant_plan->p_newpos ));
				if(ret != 0){
					wt_bl_debug_print(NULL, "DAP1:backup old failer.\r\n");
					return -12;
				}

				ret = patch_extra(ant_plan, decode_buf, temp);
				if(ret != 0){
					wt_bl_debug_print(NULL, "DAP1: patch extra error:%d.\r\n", -ret);
					return -13;
				}
			}

			remain -= temp;
			ant_plan->p_newpos += temp;

			if(remain == 0)break;
		}

		ant_plan->p_oldpos += (int32_t)ctrl[2];
		ant_plan->b_curr_block++;

	}

//	wt_bl_debug_print(NULL, "search right pos, newpos is %d, old pos is %d.\r\n",ant_plan->p_oldpos, ant_plan->p_oldpos);
exit:
	return 0;

}

static int clear_patch_status(decode_f_man *dec_info, ant_plan_t *ant_plan)
{
	int ret =0;
	
	wt_bl_debug_print(NULL, "clear_patch_status\r\n");
	ret = arch_erase_flash(ant_plan->bk_magic_addr, ant_plan->f_sector_size);

	return ret;
}

//测试各种各样的数据是否一致
static int test_data(decode_f_man *dec_info, ant_plan_t *ant_plan)
{
	int ret=0;

	wt_bl_debug_print(NULL, "********data struct check start*********.\r\n");
	wt_bl_debug_print(NULL, "b_diff_ctrl[] size is %d.\r\n", sizeof(ant_plan->b_diff_ctrl));
//	wt_bl_debug_print(NULL, "align data: %8x, %8x.\r\n", FLASH_SECTOR_ALIGN, ~((uint32_t)(F_SECTORE_SIZE-1)));
	wt_bl_debug_print(NULL, "uint8:%d,uint32_t:%d,uint64_t:%d,int64_t:%d.\r\n",sizeof(uint8_t),sizeof(uint32_t),sizeof(uint64_t),sizeof(int64_t));
	wt_bl_debug_print(NULL, "********data struct check end*********.\r\n");
	return ret;
}


// firmware head info
// 16bytes 		 | 13byte lzma header | 	compressed data N bytes
//|8magic|8vers	 |	  				  | topatch block header | topatch array | diff firmware
//									  | magic head | length	 | 				 | diff size | extra size | redirect offs | diff firmware
int firmware_patch(flash_handle_t *f_handle, flash_des_t *flash_info)
{
	int ret=0,i;
	uint8_t tdata[128];
	uint32_t unpackSize;
	uint32_t rd_flash_addr=0;
	decode_f_man dec_info;
	ant_plan_t *ant_p;

	ant_p = &ant_plan_info;
	//1. set configure
	ret = set_all_config(ant_p, f_handle, flash_info);	
	if(ret != 0){
		wt_bl_debug_print(NULL, "config failed.\r\n");
		return -1;
	}
	
	rd_flash_addr = ant_p->f_dw_addr;

	ret = arch_read_flash(rd_flash_addr, tdata, LZMA_PROPS_SIZE + 8);
	if(ret != 0){
		wt_bl_debug_print(NULL, "read flash error1.\r\n");
		return -1;
	}
	rd_flash_addr += (LZMA_PROPS_SIZE + 8);


	/*
	LZMA compressed file format
	---------------------------
	Offset Size Description
	  0     1   Special LZMA properties (lc,lp, pb in encoded form)
	  1     4   Dictionary size (little endian)
	  5     8   Uncompressed size (little endian). -1 means unknown size
	 13         Compressed data
	 */
	unpackSize = 0;
	for (i = 0; i < 8; i++)
		unpackSize += (uint32_t)tdata[LZMA_PROPS_SIZE + i] << (i * 8);
	//if(0){//(unpackSize > ant_p->f_max_fw_size){
		//不判断未压缩文件大小
	//	wt_bl_debug_print(NULL, "ERROR:unpacksize is%d, fw_max is %d !!!\n",unpackSize,ant_p->f_max_fw_size);
	//	return -2;
	//}

	totol_Size = unpackSize;
	memset((uint8_t*)&dec_info, 0, sizeof(dec_info));
    LzmaDec_Construct(&dec_info.state);
    LzmaDec_sm_Allocate(&dec_info.state, tdata, LZMA_PROPS_SIZE, (ISzAlloc *)&lzma_alloc);
    wt_bl_debug_print(NULL, "\nLZMA decode begin: \n");
    wt_bl_debug_print(NULL, "destination = %x, dest size =%d.\n", ant_p->f_fw_addr,ant_p->f_max_fw_size);
    wt_bl_debug_print(NULL, "source = %x\n", rd_flash_addr);
    wt_bl_debug_print(NULL, "unpacked_size = %d\n\n\n", unpackSize);

    LzmaDec_sm_Init(&dec_info.state);

    dec_info.src_start 	= rd_flash_addr;
    dec_info.src_curr	= dec_info.src_start;
    dec_info.src_end	= ant_p->f_dw_addr + F_DOWN_SIZE;
    dec_info.unpackSize = unpackSize;
    dec_info.inbuf		= lzma_buf_in;
    dec_info.outbuf		= lzma_buf_out;
    dec_info.inbuf_size = IN_BUF_SIZE;
    dec_info.outbuf_size= OUT_BUF_SIZE;


    //获取topatch chain, this is private add to bsdiff.
    ret = get_topatch_chain(&dec_info, ant_p);
    if(ret != 0){
    	wt_bl_debug_print(NULL, "get_to_patch_chain_lzma error -%d.\r\n",-ret);
		return -3;
    }


    test_data(&dec_info, ant_p);


    //check whether there is an unfinished patch process;    
    ret = decode_and_patch(&dec_info, ant_p);
    if(ret != 0){
    	wt_bl_debug_print(NULL, "decode and patch error,ret is -%d\r\n",-ret);		
		mi_iot_set_fota_status(UPSTATE_IDLE);
	}

    //TODO, patch完成后要清除相应的标志和status in flash
    //TODO, patch完成后也应该做校验
    ret =clear_patch_status(&dec_info, ant_p);

	return ret;
}
