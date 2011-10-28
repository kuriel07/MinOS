/*
 * kernel.h
 * this is machine independent code for os engine
 *  Created on: Nov 27, 2010
 *      Author: Agus Purwanto
 */

#ifndef _MINOS_CORE_H_
#include "defs.h"
#include "minos_config.h"

typedef struct Task Task;
struct Task
{
	uchar PID;						//current task id (PID = Process Identifier)
	uchar State;					//current task state (DORMANT, READY, WAIT, RUNNING)
	uint16 Priority;				//task priority on preemption active
#if (MPU_ARCH == MPU_ARCH_16)
	uint16 MaxPeriod;				//task timeout(in tick)
	uint16 CurrentPeriod;			//task tick counter
	uint16 CounterTick;
#elif (MPU_ARCH == MPU_ARCH_32)
	uint32 MaxPeriod;				//task timeout(in tick)
	uint32 CurrentPeriod;			//task tick counter
	uint32 CounterTick;
#endif
	lp_void Stack;
	struct Task * Next;
};

/*******************************************************************************************/
/* void _os_init();                                                                        */
/* parameter : (void), return : (void)                                                     */
/*******************************************************************************************/
void _os_init(void);
/*******************************************************************************************/
/* void _os_force_switch(pid);                                                             */
/* parameter : pid, return : (void)                                                        */
/*******************************************************************************************/
void _os_force_switch(uchar pid);
#if (MPU_ARCH == MPU_ARCH_16)
/*******************************************************************************************/
/* void _os_create_task(task_vector, priority, stack_size, timeout);                       */
/* parameter : task_vector, priority, stack_size, timeout, return : (void)                 */
/*******************************************************************************************/
void _os_create_task(uchar pid, void (* task_vector), uint16 priority, uint16 stack_size, uint16 timeout);
/*******************************************************************************************/
/* uint16 _os_tick_isr(sp);                                                                */
/* parameter : stack pointer, return : stack pointer                                       */
/*******************************************************************************************/
uint16 _os_tick_isr(uint16 sp);
/*******************************************************************************************/
/* uint32 _os_wait(tick);                                                                  */
/* parameter : tick to wait, return : (void)                                               */
/*******************************************************************************************/
void _os_wait(uint16 tick);
#elif (MPU_ARCH == MPU_ARCH_32)
/*******************************************************************************************/
/* void _os_create_task(task_vector, priority, stack_size, timeout);                       */
/* parameter : task_vector, priority, stack_size, timeout, return : (void)                 */
/*******************************************************************************************/
void _os_create_task(uchar pid, void (* task_vector), uint16 priority, uint32 stack_size, uint32 timeout);
/*******************************************************************************************/
/* uint32 _os_tick_isr(sp);                                                                */
/* parameter : stack pointer, return : stack pointer                                       */
/*******************************************************************************************/
uint32 _os_tick_isr(uint32 sp);
/*******************************************************************************************/
/* uint32 _os_wait(tick);                                                                  */
/* parameter : tick to wait, return : (void)                                               */
/*******************************************************************************************/
void _os_wait(uint32 tick);
#endif

uchar _os_getpid(void);
void _os_assign_new_task(Task * task);
void _os_change_priority(uchar pid, uint16 priority);
void _os_kill_task(uchar pid);
void _os_force_switch(uchar pid);

/* internal task states */
#define DORMANT			0x01
#define READY			0x02
#define RUNNING			0x04
#define WAIT			0x08
#define IDLE			0x10
#define HALT			0x80

#define _MINOS_CORE_H_
#endif /* KERNEL_H_ */
