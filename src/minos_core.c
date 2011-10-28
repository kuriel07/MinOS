/*
 * core.c
 *
 *  Created on: Nov 27, 2010
 *      Author: Agus Purwanto
 */
#include "defs.h"
#include "minos_core.h"
#include "minos_config.h"

Task * _active_task;
Task * _highest_task, * _lowest_task;
#if (MPU_ARCH == MPU_ARCH_16)
uint16 _os_max_tick = 0;
#elif (MPU_ARCH == MPU_ARCH_32)
uint32 _os_max_tick = 0;
#endif

//local prototypes
Task * _os_get_task_by_pid(uchar pid);
Task * _os_get_previous_task(Task * task);

void _os_idle_loop();
void _os_idle_task(void);
void _os_interpret_command(uchar c);
void _os_system_idle_task(void);

#if RTK_ENABLE_MONITOR
#include "stdio.h"
uchar _os_mon_queue[RTK_MONITOR_BUFSIZE];
void _os_mon_tick(void) {
	#if (MPU_ARCH == MPU_ARCH_16)
	uint16 i = 0;
	_os_mon_putc('s');					//sync byte
	_os_mon_putc(_os_max_tick >> 8);
	_os_mon_putc((uchar)_os_max_tick);
	#elif (MPU_ARCH == MPU_ARCH_32)
	uint32 i = 0;
	_os_mon_putc('S');					//sync byte
	_os_mon_putc(_os_max_tick >> 24);
	_os_mon_putc(_os_max_tick >> 16);
	_os_mon_putc(_os_max_tick >> 8);
	_os_mon_putc((uchar)_os_max_tick);
	#endif
	for(i=0; i<_os_max_tick; i++) {
		_os_mon_putc(_os_mon_queue[i]);
	}
}
#endif

#if (MPU_ARCH == MPU_ARCH_16)
uint16 _os_tick_isr(uint16 sp) {
	static uint16 _os_tick = 0;
#elif (MPU_ARCH == MPU_ARCH_32)
uint32 _os_tick_isr(uint32 sp) {
	static uint32 _os_tick = 0;
#endif
	static Task * _task_iterator;
#if	RTK_ENABLE_PREEMPTION
	static Task * _task_candidate;
	_task_candidate = NULL;
#endif
	_os_tick++;
	//save stack pointer
	_task_iterator = _highest_task;
	while(_task_iterator != NULL) {
		if(_task_iterator->State == WAIT) {
			if(_task_iterator->CounterTick != 0) {
				_task_iterator->CounterTick --;					//count down tick
			} else {
				_task_iterator->State = READY;
			}
		}
		//enable preemption
#if RTK_ENABLE_PREEMPTION
		if(_task_iterator->State == READY && _task_candidate == NULL) {
			_task_candidate = _task_iterator;
		}
#endif
		//end of preemption
		_task_iterator = _task_iterator->Next;
	}
	//enable preemption
#if	RTK_ENABLE_PREEMPTION
	if(_task_candidate != NULL) {
		_active_task->CurrentPeriod = 0;		//reset current period
		_active_task->State = WAIT;				//set state to wait(halt task)
		_active_task->Stack = sp;				//save stack pointer
		_active_task = _task_candidate;			//set to candidate
	}
#endif
	//end of preemption
	loop_context_switch:
	if(_active_task != NULL) {
		switch(_active_task->State) {
			case DORMANT:
				_active_task->State = READY;
				_active_task->Stack = _os_save_context_safe(_active_task->Stack);
			case READY:
				_active_task->State = RUNNING;
				sp = _active_task->Stack;
				//SP = _active_task->Stack;
				break;
			case WAIT:
				_active_task = _active_task->Next;			//switch task (not ready yet)
				goto loop_context_switch;					//loop until ready task found
			case HALT:
				_active_task = _active_task->Next;			//this task is halted
				goto loop_context_switch;					//loop until ready task found
			case RUNNING:
				if(++_active_task->CurrentPeriod < _active_task->MaxPeriod) {
					/*if(_active_task->PID == 0) {
						_active_task->CurrentPeriod = 0;
					}*/
					_active_task->Stack = sp;
					break;
				} else {
					_active_task->CurrentPeriod = 0;		//reset current period
					_active_task->State = WAIT;				//set state to wait(halt task)
					_active_task->Stack = sp;				//save stack pointer
					_active_task = _active_task->Next;		//switch task
					sp = _active_task->Stack;
					goto loop_context_switch;				//loop until ready task found
				}
			default:
				break;
		}
	} else {
		//idle information (run lowest priority task)
		sp = _lowest_task->Stack;
	}
	//reset kernel tick
	if(_os_tick > _os_max_tick) {
		_os_tick = 0;
		_active_task->State = WAIT;				//set state to wait(halt task)
		_active_task->Stack = sp;				//save stack pointer
		_active_task = _highest_task;
		sp = _active_task->Stack;
#if RTK_ENABLE_MONITOR
		_os_mon_tick();
#endif
		goto loop_context_switch;
	}
#if RTK_ENABLE_MONITOR
	_os_mon_queue[_os_tick] = _active_task->PID;
#endif
	return _active_task->Stack;
}

void _os_idle_loop() {
	uint16 i = 0;
	i = i;
	return;
}

void _os_idle_task(void) {
	while(1) {
		_active_task->CurrentPeriod = 0;		//no timeout
		_os_idle_loop();
	}
}

void _os_interpret_command(uchar c) {
	Task * current_task;
	Task * task_iterator;
	uchar total_task;
	switch(c) {
		case 'H':		//[H]HALT [PID]
			c = _os_mon_getc();
			current_task = _os_get_task_by_pid(c);
			current_task->State = HALT;
			break;
		case 'R':		//[R]RESUME [PID]
			c = _os_mon_getc();
			current_task = _os_get_task_by_pid(c);
			if(current_task->State == HALT) {
				current_task->CounterTick = 0;
				current_task->State = READY;
			}
			break;
		case 'L':		//[L]PROCESS LIST
			_os_mon_putc('P');
			total_task = 0;
			task_iterator = _highest_task;
			while(task_iterator != NULL) {
				total_task ++;
				task_iterator = task_iterator->Next;
			}
			_os_mon_putc(total_task);
			task_iterator = _highest_task;
			while(task_iterator != NULL) {
				_os_mon_putc(task_iterator->PID);
				task_iterator = task_iterator->Next;
			}
		default:
			break;
	}
}

void _os_system_idle_task(void) {
	uchar supervisor_mode = FALSE;
	uchar c;
	while(1) {
		_active_task->CurrentPeriod = 0;		//no timeout
		c = _os_mon_getc();
		if(supervisor_mode) {
			if(c == 'M') {						//enter supervisor mode
				_os_enter_critical();
				supervisor_mode = TRUE;
				_os_mon_putc('A');				//send acknowledge
			}
			else if(c == 'E') {
				_os_mon_putc('A');				//send acknowledge
				supervisor_mode = FALSE;
				_os_exit_critical();
			}
			else {
				_os_interpret_command(c);
			}
		} else {
			if(c == 'M') {						//enter supervisor mode
				_os_enter_critical();
				supervisor_mode = TRUE;
				_os_mon_putc('A');				//send acknowledge
			}
		}
	}
}

void _os_init(void) {
	//initialize memory allocator
#if RTK_USE_MIDGARD
	m_init_alloc();
#endif
	//initialize timer interrupt
	_os_init_machine();
	_active_task = NULL;
	_highest_task = _lowest_task = _active_task;
	//create lowest priority task (idle task)
	_os_create_task(0xff, _os_idle_task, 0xffff, 0x60, 0xffff);
	_os_create_task(0, _os_system_idle_task, 0xfffe, 0x90, 0xffff);
	_active_task = _highest_task;
	_os_max_tick = 0;
}

#if (MPU_ARCH == MPU_ARCH_16)
void _os_create_task(uchar pid, void (* task_vector), uint16 priority, uint16 stack_size, uint16 timeout) {
	Task * new_task;
	uchar * byte_pointer;		//prevent memory access alignment error on several machine
#if RTK_USE_MIDGARD
	//create new task at heap, use midgard memory manager
	new_task = (Task *)m_alloc(sizeof(Task));
	//initialize task stack pointer to heap max pointer
	new_task->Stack = (lp_void)m_alloc(stack_size) + stack_size - sizeof(lp_void);
#else
	//create new task at heap, use default ansi C memory allocator
	new_task = (Task *)malloc(sizeof(Task));
	//initialize task stack pointer to heap max pointer
	new_task->Stack = (lp_void)malloc(stack_size) + stack_size - sizeof(lp_void);
#endif
	byte_pointer = (uchar *)new_task->Stack;
	#if (MPU_ENDIANESS == MPU_ADDR_LITTLE_ENDIAN)
	*(byte_pointer + 1) = (uchar)((lp_void)task_vector);
	*(byte_pointer --) = (uchar)((lp_void)task_vector >> 8);
	new_task->Stack = (lp_void)byte_pointer;
	#elif (MPU_ENDIANESS == MPU_ADDR_BIG_ENDIAN)
	*(byte_pointer + 1) = (uchar)((lp_void)task_vector >> 8);
	*(byte_pointer --) = (uchar)((lp_void)task_vector);
	new_task->Stack = (lp_void)byte_pointer;
	#endif
#elif (MPU_ARCH == MPU_ARCH_32)
void _create_task(uchar pid, void (* task_vector), uint16 priority, uint32 stack_size, uint32 max_period) {
	Task * new_task;
	Task * task_iterator;
	Task * prev_task = NULL;
	uchar * byte_pointer;		//prevent memory access alignment error on several machine
	//create new task at heap, use midgard memory manager
	new_task = (Task *)m_alloc(sizeof(Task));
	//initialize task stack pointer to heap max pointer
	new_task->Stack = (lp_void)m_alloc(stack_size) + stack_size - sizeof(lp_void);
	byte_pointer = (uchar *)new_task->Stack;
	#if (MPU_ENDIANESS == MPU_ADDR_LITTLE_ENDIAN)
	*(byte_pointer + 3) = (uchar)((lp_void)task_vector);
	*(byte_pointer + 2) = (uchar)((lp_void)task_vector>>8);
	*(byte_pointer + 1) = (uchar)((lp_void)task_vector>>16);
	*(byte_pointer --) = (uchar)((lp_void)task_vector>>24);
	new_task->Stack = (lp_void)byte_pointer;
	#elif (MPU_ENDIANESS == MPU_ADDR_BIG_ENDIAN)
	*(byte_pointer + 3) = (uchar)((lp_void)task_vector>>24);
	*(byte_pointer + 2) = (uchar)((lp_void)task_vector>>16);
	*(byte_pointer + 1) = (uchar)((lp_void)task_vector>>8);
	*(byte_pointer --) = (uchar)((lp_void)task_vector);
	new_task->Stack = (lp_void)byte_pointer;
	#endif
#endif
	//set task state to dormant
	new_task->PID = pid;
	new_task->Priority = priority;
	new_task->State = DORMANT;
	new_task->MaxPeriod = timeout;
	new_task->CurrentPeriod = 0;
	new_task->CounterTick = 0;
	new_task->Next = NULL;
	_os_max_tick += timeout;				//calculate all tasks period
	_os_assign_new_task(new_task);
	_active_task = _highest_task;
}

#if (MPU_ARCH == MPU_ARCH_16)
void _os_wait(uint16 tick) {
#elif (MPU_ARCH == MPU_ARCH_32)
void _os_wait(uint32 tick) {
#endif
	_active_task->CurrentPeriod = _active_task->MaxPeriod;
	_active_task->CounterTick = tick;
	_os_tick();
	//while(_active_task->CounterTick != 0);
}

//select task from pcb using pid
Task * _os_get_task_by_pid(uchar pid) {
	Task * task_iterator;
	task_iterator = _highest_task;
	while(task_iterator != NULL) {
		if(task_iterator->PID == pid) {
			return task_iterator;
		}
		//iterate next task
		task_iterator = task_iterator->Next;
	}
	return NULL;
}

//get previous task on pcb
Task * _os_get_previous_task(Task * task) {
	Task * task_iterator;
	task_iterator = _highest_task;
	while(task_iterator != NULL) {
		if(task_iterator->Next == task) {
			return task_iterator;
		}
		//iterate next task
		task_iterator = task_iterator->Next;
	}
	return NULL;
}

//assign new task on pcb depend on its priority
void _os_assign_new_task(Task * task) {
	Task * task_iterator;
	Task * prev_task = NULL;
	task_iterator = _highest_task;
	while(task_iterator != NULL) {
		if(task_iterator->Priority > task->Priority) break;
		prev_task = task_iterator;
		task_iterator = task_iterator->Next;
	}
	if(prev_task != NULL) {
		prev_task->Next = task;
	} else {
		_highest_task = task;
	}
	if(task_iterator != NULL) {
		task->Next = task_iterator;
	} else {
		_lowest_task = task;
	}
}

void _os_change_priority(uchar pid, uint16 priority) {
	Task * current_task;
	Task * previous_task;
	//enter critical section here
	_os_enter_critical();
	current_task = _os_get_task_by_pid(pid);
	previous_task = _os_get_previous_task(current_task);
	previous_task->Next = current_task->Next;
	current_task->Next = NULL;
	current_task->Priority = priority;
	_os_assign_new_task(current_task);
	//exit critical section here
	_os_exit_critical();
}

//delete task on kernel time (might buggy)
void _os_kill_task(uchar pid) {
	Task * current_task;
	Task * previous_task;
	//enter critical section here
	_os_enter_critical();
	current_task = _os_get_task_by_pid(pid);
	previous_task = _os_get_previous_task(current_task);
	previous_task->Next = current_task->Next;
	_os_max_tick -= current_task->MaxPeriod;
#if RTK_USE_MIDGARD
	m_free(current_task);
#else
	free(current_task);
#endif
	//exit critical section here
	_os_exit_critical();
}

//get pid of current running task
uchar _os_getpid(void) {
	return _active_task->PID;
}

//force switch task immedietly to current pid
void _os_force_switch(uchar pid) {
	uint16 current_priority;
	Task * _task_iterator;
	_task_iterator = _highest_task;
	while(_task_iterator != NULL) {
		if(_task_iterator->PID == pid) {					//check if task exist
			current_priority = _task_iterator->Priority;
			_task_iterator->CounterTick = 0;
			_task_iterator->State = READY;
			_task_iterator->Priority = 0;					//highest priority
			_os_assign_new_task(_task_iterator);			//assign priority to pcb
			_os_tick();										//force kernel tick
			_task_iterator->Priority = current_priority;	//set to previous priority
			_os_assign_new_task(_task_iterator);			//assign priority to pcb
			return;
		}
		//iterate next task
		_task_iterator = _task_iterator->Next;
	}
}
