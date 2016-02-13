/***********************************************************
	ЯДОРО RTOS
	Автор DI-HALT http://easyelectronics.ru/
***********************************************************/

#ifndef EERTOSHAL_H
#define EERTOSHAL_H

#include <avr\interrupt.h>
#include "avrlibtypes.h"
#include "Clock.h"

#define STATUS_REG 			SREG
#define Interrupt_Flag		SREG_I
#define Disable_Interrupt	cli()
#define Enable_Interrupt	sei()
/*
//Таймер для службы таймера - таймер 2
//Настройка службы таймера
#define Prescaler	  		128
#define	TimerDivider  		(F_CPU/Prescaler/1000)		// значение регистра сравнения для получения 1 миллисекунды 

#define SERVICE_TIMER_TCCR 	TCCR2 						// Номер таймера для службы таймера
#define SERVICE_TIMER_CTC 	WGM21 						// Бит режима сброса по сравнению
#define SERVICE_TIMER_CS0	CS20 						// Бит 0 настройка предделителя
#define SERVICE_TIMER_CS1	CS21 						// Бит 1 настройка предделителя
#define SERVICE_TIMER_CS2	CS22 						// Бит 2 настройка предделителя

#define SetTimerCTCMode		SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CTC)
#if (Prescaler == 64)
	#define SetTimerPresclr	SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CS2) | (0<<SERVICE_TIMER_CS1) | (0<<SERVICE_TIMER_CS0)	//64
#elif (Prescaler == 128)
	#define SetTimerPresclr	SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CS2) | (0<<SERVICE_TIMER_CS1) | (1<<SERVICE_TIMER_CS0)	//128
#else
	#warning "Prescaller not define!"
#endif

#define SERVICE_TIMER_TCTNT TCNT2 						// Регистр начального значения счетчика
#define SERVICE_TIMER_OCR 	OCR2 						// Регистр сравнения
#define SERVICE_TIMER_TMISK	TIMSK 						// Регистр маски прерываний
#define SERVICE_TIMER_OCIE	OCIE2						// Бит разрешения прерывания по совпадению
#define RTOS_ISR  			TIMER2_COMP_vect			// Прерывание для работы ядра
*/

#define	TaskQueueSize		20		/* Размер очереди задач диспетчера */
#define MainTimerQueueSize	15		/* Размер очереди службы таймера */

extern void RunRTOS (void);

#endif
