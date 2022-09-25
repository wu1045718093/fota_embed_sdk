#ifndef _SM_PLATFORM_H_
#define _SM_PLATFORM_H_
#include <stdarg.h>
#include "sm_datatype.h"
#include "stdio.h"

typedef void (*FuncPtr) (void);

const sm_s8* sm_strstr(const sm_s8* src, const sm_s8* sub);

sm_s32 sm_vsnprintf(sm_s8 * buf, sm_u32 bufSize, const sm_s8 * format, va_list arglist);

sm_s8* sm_strcpy(char* dest, const char* src);

sm_s8* sm_strcat(char* dest, const char* src);

sm_u32 sm_strlen(const char* src);

const sm_s8* sm_strchr(const sm_s8* src, int chr);

sm_s32 sm_strcmp(const char* str1, const char* str2);

void* sm_memcpy(void* dest, const void* src, sm_u32 size);

void* sm_memset(void * dest, sm_s32 val, sm_u32 size);

sm_s32 sm_memcmp(const void* src, const void* dest, sm_s32 size);

sm_s32 sm_sprintf(sm_s8 * buf, const char * format, ...);

sm_s32 sm_atoi(const char* str);

void* sm_malloc_porting(sm_u32 allocSize);

void  sm_free_porting(void* memBlock);

sm_s32 wt_start_timer(sm_u32 timer_id,
						  sm_u32 timer_expire,
						  FuncPtr callback);

sm_s32 wt_start_timer_ex(sm_u32 timer_id,
							 sm_u32 timer_expire,
							 FuncPtr callback,
							 void *user_data);

void wt_stop_timer(sm_u32 timer_id);

#endif
