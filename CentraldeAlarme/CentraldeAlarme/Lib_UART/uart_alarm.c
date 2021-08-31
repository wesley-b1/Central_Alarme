/*
* uart_alarme.c
*
 * Created: 26/08/2021 18:07:01
 *  Author: Sthefania
 */ 

#include <avr/io.h>
#include "uart_alarm.h"

void uart_config()
{
	UBRR0=103; //baud rate=9600; modo normal, freq=16MHz
	UCSR0B=8; // TXEN0=1
	UCSR0C=6; // UCSZ01=UCSZ00=1
	
}

void uart_enviachar(unsigned char a)
{
	UDR0 = a;
	//Aguarda o buffer ser desocupado
	while (! (UCSR0A & (1<<UDRE0)) );
}

void uart_enviaword(char *s)
{
	unsigned int i=0;
	while (s[i] != '\x0')
	{
		uart_enviachar(s[i++]);
	};
}