/***********************************************************
	ЯДОРО RTOS
	Автор DI-HALT http://easyelectronics.ru/
***********************************************************/
#include "EERTOSHAL.h"

//RTOS Запуск системного таймера
inline void RunRTOS (void)
{
/* Запуск RTOS теперь не требует запуска таймера, т.к. используется прерывание от 32 кГц
SetTimerCTCMode;								// Freq = CK/64 - Установить режим и пред делитель
SetTimerPresclr;
												// Авто сброс после достижения регистра сравнения
SERVICE_TIMER_TCTNT = 0;						// Установить начальное значение считчиков
SERVICE_TIMER_OCR  = TimerDivider;				// Установить значение в регистр сравнения
SERVICE_TIMER_TMISK |= (1<<SERVICE_TIMER_OCIE);	// Разрешаем прерывание по сравнению 
*/
Enable_Interrupt;								//Разрешаем прерывания RTOS - запуск ОС
}
