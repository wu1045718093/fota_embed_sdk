#ifndef _SM_DATATYPE_H_
#define _SM_DATATYPE_H_

#ifndef NULL
#define NULL (void*)0
#endif

#if 1
typedef signed int 	 		int32_t;
typedef unsigned int 		uint32_t;

typedef unsigned short 		uint16_t;
typedef unsigned char 		uint8_t;

typedef signed long long 	s64_t;
typedef unsigned long long 	u64_t;
typedef signed long long 	int64_t;
typedef unsigned long long 	uint64_t;
typedef signed long long    int64_t;

/** Update Agent Boolean Definition */
typedef int              mi_bool;

/** True Definition */
#define mi_true               0x1

/** False Definition */
#define mi_false              0x0

#endif

#endif
