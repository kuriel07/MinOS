#ifndef _MINOS_CONFIG_H

//enable preemption, default cooperative multitasking
#define RTK_ENABLE_PREEMPTION		1
#define RTK_USE_MIDGARD				0
#define RTK_STACK_SIZE				850						//amount bytes of all stacks

#define RTK_ENABLE_MONITOR			1
#define RTK_MONITOR_BUFSIZE			64

#define MPU_ARCH_16					0xc16
#define MPU_ARCH_32					0xc32
#define MPU_ADDR_BIG_ENDIAN			0xa08
#define MPU_ADDR_LITTLE_ENDIAN		0xa80

#define MPU_ARCH					MPU_ARCH_16					//16 bit address
#define MPU_ENDIANESS				MPU_ADDR_LITTLE_ENDIAN		//little endian

#if (MPU_ARCH == MPU_ARCH_16)		//different pointer size for 16/8 bit machine (16 bit addressing)
typedef unsigned short lp_void;
#elif (MPU_ARCH == MPU_ARCH_32)		//different pointer size for 32 bit machine (32 bit addressing)
typedef unsigned long lp_void;
#endif

#include "minos_machine.h"
#if RTK_USE_MIDGARD
#include "midgard.h"
#endif
#define _MINOS_CONFIG_H
#endif
