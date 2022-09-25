#ifndef __SM_ERROR_H__
#define __SM_ERROR_H__

#define SM_ERR_FAILED                      -1  // 未定义类型错误
#define SM_ERR_OK                           0   // 成功

#define SM_ERR_FLASH_INIT                   1  // flash初始化错误
#define SM_ERR_FLASH_WRITE                  2  // falsh写入失败
#define SM_ERR_FLASH_READ                   3  // flash读取失败
#define SM_ERR_WRITE_DATA_FAILE             4  // 文件写入数据错误
#define SM_ERR_READ_DATA_FAILE              5  // 文件读取数据错误
#define SM_ERR_INVALID_PARAM                6  // 无效参数
#define SM_ERR_WRITE_NV_FAILE             	7  // NV写入数据错误
#define SM_ERR_READ_NV_FAILE             	8  // NV读取数据错误
#define SM_ERR_MALLOC_MEM_FAILE             9  // 申请内存失败
#define SM_ERR_INVALID_URL                  10 // 无效URL地址
#define SM_ERR_XPT_GET_HOST_BY_NAME         11 // 域名解析失败
#define SM_ERR_SPACE_NOT_ENGHOU				12 // 空间是否充足
#define SM_ERR_DOWNLOAD_BUSY				13 // 有下载任务正在执行




#endif
