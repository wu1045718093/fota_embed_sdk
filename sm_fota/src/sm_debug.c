#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdio.h>
#include <stdarg.h>
#include "MMI_features.h"
#include "kal_release.h"
#include "sm_datatype.h"





void sm_DebugPrint(const char *content, ...)
{
	va_list Args;
	sm_s8 out_buf[512] = {0};
	sm_s32 len;
	
	va_start(Args,content);
	len = vsnprintf(out_buf,512, content, Args);

#ifndef WIN32
	kal_prompt_trace(MOD_BT, "%s\n", out_buf);
#else
	OutputDebugStringA(out_buf);
	OutputDebugStringA("\n");
#endif
	va_end(Args);
}

