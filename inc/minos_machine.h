#ifndef _MINOS_MACHINE_H
//this is machine dependent code for atmega series
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "defs.h"

#define XTAL 11059200
#define KERNEL_TICK ((XTAL / 5000) - 128)

/** START OF KERNEL INTERNAL PROCESS, FOR ATMEGA SERIES PORTABILITY PURPOSE **/
#define _os_init_machine() { \
	/*normal port OC1A and OC1B operation*/ \
	TCCR1A = 0x00; \
	/*timer mask, activate interrupt timer 1 match A*/ \
	TIMSK |= 0x10; \
	/*timer flag clear*/ \
	TIFR = 0x00; \
	/*default compare value*/ \
	TCNT1 = 0x0000; \
	OCR1A = KERNEL_TICK; \
	/*timer prescaler = fosc/64*/ \
	TCCR1B &= 0xF0; \
	TCCR1B |= 0x0B; \
    /* USART Receiver: On */ \
    /* USART Transmitter: On */ \
    /* USART Mode: Asynchronous */ \
    /* USART Baud Rate: 19200 */ \
    UCSRA=0x00; \
    UCSRB=0x18; \
    UCSRC=0x06; \
    UBRRH=0; \
    UBRRL=0x23; \
}

//force kernel to switch task
#define _os_tick() {\
	TIFR |= 0x10;\
}

#define _os_enter_critical() {\
	TIMSK &= ~0x10;\
}

#define _os_exit_critical() {\
	TIMSK |= 0x10;\
}

//full registers saving context
#define _save_context() {\
	asm volatile (\
			"push	r0						\n\t"	\
			"in		r0, __SREG__			\n\t"	\
			"cli							\n\t"	\
			"push	r0						\n\t"	\
			"push	r1						\n\t"	\
			"clr	r1						\n\t"	\
			"push	r2						\n\t"	\
			"push	r3						\n\t"	\
			"push	r4						\n\t"	\
			"push	r5						\n\t"	\
			"push	r6						\n\t"	\
			"push	r7						\n\t"	\
			"push	r8						\n\t"	\
			"push	r9						\n\t"	\
			"push	r10						\n\t"	\
			"push	r11						\n\t"	\
			"push	r12						\n\t"	\
			"push	r13						\n\t"	\
			"push	r14						\n\t"	\
			"push	r15						\n\t"	\
			"push	r16						\n\t"	\
			"push	r17						\n\t"	\
			"push	r18						\n\t"	\
			"push	r19						\n\t"	\
			"push	r20						\n\t"	\
			"push	r21						\n\t"	\
			"push	r22						\n\t"	\
			"push	r23						\n\t"	\
			"push	r24						\n\t"	\
			"push	r25						\n\t"	\
			"push	r26						\n\t"	\
			"push	r27						\n\t"	\
			"push	r28						\n\t"	\
			"push	r29						\n\t"	\
			"push	r30						\n\t"	\
			"push	r31						\n\t"	\
	);\
}

//full registers restore context
#define _restore_context() {\
	asm volatile (\
			"pop	r31						\n\t"	\
			"pop	r30						\n\t"	\
			"pop	r29						\n\t"	\
			"pop	r28						\n\t"	\
			"pop	r27						\n\t"	\
			"pop	r26						\n\t"	\
			"pop	r25						\n\t"	\
			"pop	r24						\n\t"	\
			"pop	r23						\n\t"	\
			"pop	r22						\n\t"	\
			"pop	r21						\n\t"	\
			"pop	r20						\n\t"	\
			"pop	r19						\n\t"	\
			"pop	r18						\n\t"	\
			"pop	r17						\n\t"	\
			"pop	r16						\n\t"	\
			"pop	r15						\n\t"	\
			"pop	r14						\n\t"	\
			"pop	r13						\n\t"	\
			"pop	r12						\n\t"	\
			"pop	r11						\n\t"	\
			"pop	r10						\n\t"	\
			"pop	r9						\n\t"	\
			"pop	r8						\n\t"	\
			"pop	r7						\n\t"	\
			"pop	r6						\n\t"	\
			"pop	r5						\n\t"	\
			"pop	r4						\n\t"	\
			"pop	r3						\n\t"	\
			"pop	r2						\n\t"	\
			"pop	r1						\n\t"	\
			"pop	r0						\n\t"	\
			"out	__SREG__, r0			\n\t"	\
			"pop	r0						\n\t"	\
	);\
}
/** END OF KERNEL INTERNAL PROCESS, FOR ATMEGA SERIES PORTABILITY PURPOSE **/
uint16 _os_save_context_safe(uint16 sp);
void _os_mon_putc(uchar c);
uchar _os_mon_getc(void);

#define _MINOS_MACHINE_H
#endif
