

#ifndef _SM_DATATYPE_H_
#define _SM_DATATYPE_H_

#ifndef __SMPLATFORM_H__
#define __SMPLATFORM_H__

typedef signed char			sm_s8;
typedef signed short		sm_s16;
typedef signed long			sm_s32;
typedef unsigned char		sm_u8;
typedef unsigned short		sm_u16;
typedef unsigned long		sm_u32;
typedef char				sm_char;
typedef void				sm_void;
typedef void*				sm_ptr;
typedef const void*			sm_cptr;

#endif

/* boolean representation */
typedef enum 
{
    /* FALSE value */
    SM_FALSE,
    /* TRUE value */
    SM_TRUE
} sm_bool;

#ifndef SM_NULL
#define SM_NULL            0
#endif

#endif