#include <stdio.h>
#include <avr\io.h>

#define BAUD 9600 //Для отладки вывод наружу

#ifndef F_CPU
	#warning "F_CPU not define"
#endif

extern int uart_putchar(char c, FILE *stream);
extern void SerilalIni(void);
