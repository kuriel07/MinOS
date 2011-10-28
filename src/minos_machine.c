#include "defs.h"
#include "minos_machine.h"
#include "minos_core.h"
#include <stdio.h>

uint16 _os_save_context_safe(uint16 sp) {
	uint16 ret_val;
	uint16 _sp = SP;
	SP = sp;
	_save_context();
	ret_val = SP;
	SP = _sp;
	return ret_val;
}

//Output Compare Match A Interrupt Vector
//round robin task switcher
ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	//disable all interrupts
	//THERE MUST BE NO PROCESS HERE
	//ENTER CRITICAL
	_save_context();
	_os_enter_critical();
	//START PROCESS
	//change task here
	SP = _os_tick_isr(SP);
	//while(1);
	//while(1);
	//END OF PROCESS
	//EXIT CRITICAL
	_os_exit_critical();
	_restore_context();
	//enable all interrupts
	//THERE MUST BE NO PROCESS HERE
	asm volatile ( "sei" );	//EXIT CRITICAL
	asm volatile ( "ret" );	//i don't believe this compiler
}

static int uart_putchar(char c, FILE *stream);

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
										 _FDEV_SETUP_WRITE);

static int uart_putchar(char c, FILE *stream)
{
  if (c == '\n')
	uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;
  return 0;
}

static int uart_getchar() {
	uchar c;
	loop_until_bit_is_set(UCSRA, RXC);
	c = UDR;
	return c;
}

void _os_mon_putc(uchar c) {
	uart_putchar(c, &mystdout);
}

uchar _os_mon_getc(void) {
	return uart_getchar();
}
