/***********************************************************
	����� RTOS
	����� DI-HALT http://easyelectronics.ru/
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
//������ ��� ������ ������� - ������ 2
//��������� ������ �������
#define Prescaler	  		128
#define	TimerDivider  		(F_CPU/Prescaler/1000)		// �������� �������� ��������� ��� ��������� 1 ������������ 

#define SERVICE_TIMER_TCCR 	TCCR2 						// ����� ������� ��� ������ �������
#define SERVICE_TIMER_CTC 	WGM21 						// ��� ������ ������ �� ���������
#define SERVICE_TIMER_CS0	CS20 						// ��� 0 ��������� ������������
#define SERVICE_TIMER_CS1	CS21 						// ��� 1 ��������� ������������
#define SERVICE_TIMER_CS2	CS22 						// ��� 2 ��������� ������������

#define SetTimerCTCMode		SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CTC)
#if (Prescaler == 64)
	#define SetTimerPresclr	SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CS2) | (0<<SERVICE_TIMER_CS1) | (0<<SERVICE_TIMER_CS0)	//64
#elif (Prescaler == 128)
	#define SetTimerPresclr	SERVICE_TIMER_TCCR |= (1<<SERVICE_TIMER_CS2) | (0<<SERVICE_TIMER_CS1) | (1<<SERVICE_TIMER_CS0)	//128
#else
	#warning "Prescaller not define!"
#endif

#define SERVICE_TIMER_TCTNT TCNT2 						// ������� ���������� �������� ��������
#define SERVICE_TIMER_OCR 	OCR2 						// ������� ���������
#define SERVICE_TIMER_TMISK	TIMSK 						// ������� ����� ����������
#define SERVICE_TIMER_OCIE	OCIE2						// ��� ���������� ���������� �� ����������
#define RTOS_ISR  			TIMER2_COMP_vect			// ���������� ��� ������ ����
*/

#define	TaskQueueSize		20		/* ������ ������� ����� ���������� */
#define MainTimerQueueSize	15		/* ������ ������� ������ ������� */

extern void RunRTOS (void);

#endif
