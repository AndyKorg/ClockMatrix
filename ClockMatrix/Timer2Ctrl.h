/*
 * Управление таймером 2 для различных задач. Предполагается, что источником импульсов является тактовый генератор МК
 * Используется прерывание переполнения описанное в OW_DELAY_INT
 * Общий принцип: Имеется флаг занятости таймера. Он взведен в те моменты когда таймер задействован в 
 * обслуживании какого-либо интерфейса.
 * В Mega таймер 2 имеет отдельный предделитель, имейте это в виду
 */ 


#ifndef TIMER2CTRL_H_
#define TIMER2CTRL_H_

#include <avr/io.h>
#include "avrlibtypes.h"

//------------- Настройка таймера
#define OW_TCNT					TCNT2								//Сам счетчик
#define OW_TCCR					TCCR2
#ifndef PSR2
#define PSR2					1									//Почему-то этот бит не описан в io.h
#endif
#define OW_PRE_RESET()			SetBit(SFIOR, PSR2)					//Сброс предделителя именно 2-ого счетчика, причем этот предделитель обслуживает только один таймер номер 2
#define OW_ENABLE_INT()			SetBit(TIMSK, TOIE2)				//Разрешить прерывание по переполнению
#define OW_DISABLE_INT()		ClearBit(TIMSK, TOIE2)				//Запретить прерывание по переполнению
#define OW_DELAY_INT			TIMER2_OVF_vect						//Таймер обеспечивающий отсчет задержек
#define OW_INTERRAPT			TOIE2								//Флаг разрешения прерывания
#define OW_DELAY_INIT			(0<<FOC2) | (0<<WGM20) | (0<<WGM21) | (0<<COM21) | (0<<COM20) //ПРИ СМЕНЕ ТАЙМЕРА ЗАМЕНИТЬ НАЗВАНИЯ БИТОВ РЕЖИМА!
#define OW_CS0					CS20								//Биты управления предделтелем
#define OW_CS1					CS21
#define OW_CS2					CS22
//------------- Маркосы управления предделителем, пример использования описан ниже
#define OW_DELAY_STOP()			OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//Остановить счетчик
#define OW_PRE64()				OW_TCCR = OW_DELAY_INIT | (1<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//Предделитель на 64
#define OW_PRE32()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (1<<OW_CS0)	//Предделитель на 32
#define OW_PRE1()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (1<<OW_CS0)	//Предделитель на 1
#define OW_BORDER				255									//Граница от которой надо отсчитывать задержки
#define OW_DELAY_VALUE(dly, div) (OW_BORDER-(F_CPU/div)/(1000000UL/dly))	//Расчет значения счетчика для конкретной задержки для конкретно предделителя
#define OW_DELAY_64(delay)	do{OW_PRE64();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 64);}while(0)
#define OW_DELAY_32(delay)	do{OW_PRE32();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 32);}while(0)
#define OW_DELAY_1(delay)	do{OW_PRE1();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 1);}while(0)
							
//------------ Объявления функций переменных ------------------------------
typedef void (*TM_OWR_EVNT)(u08 Reset);								//тип указателя на функцию обработки события окончания периода
#define FUNC_DELAY_RESET			1								//Сбросить счетчик состояния функции TM_OWR_EVNT
#define FUNC_DELAY_NORMAL			0								//Нормальная работа функции TM_OWR_EVNT в соответствии с внутренним счетчиком шагов
extern volatile TM_OWR_EVNT			EndDelayEvent;					//Ссылка на функцию вызываемую в конце задержки. Через эту функцию обеспечаивается вся работа с таймером

extern volatile u08 TimerFlag;										//Состояние автомата обслуживающего таймер

//------------ Значения флагов состояния автомата ------------------------------
#define TM_BUSY_BIT					0								//Флаг занятости таймера, взводится внешним автоматом или алгоритмом при начале работы с таймером
#define TM_FREE_BIT1				1
#define TM_FREE_BIT2				2
#define TM_FREE_BIT3				3
#define TM_FREE_BIT4				4
#define TM_FREE_BIT5				5
#define TM_FREE_BIT6				6
#define TM_FREE_BIT7				7

#define TM_BUSY()					SetBit(TimerFlag, TM_BUSY_BIT)
#define TM_FREE()					ClearBit(TimerFlag, TM_BUSY_BIT)
#define TM_IS_BUSY()				BitIsSet(TimerFlag, TM_BUSY_BIT)
#define TM_IS_FREE()				BitIsClear(TimerFlag, TM_BUSY_BIT)

//------------- Пример вычисления значений для таймера
/*
#define OW_RESET_DELAY_us			480								//Требуемая длительность - 480 мкс
#define OW_RESET_PRE				64								//Значение предделителя - выбирается вручную. 
																	//Должно быть как можно больше, что бы МК успел отработать как можно больше команд. 
																	//С другой стороны значение ограничено возможностями счетчика таймера - 255 отсчетов. Если будет больше то макрос сообщит о превышении
#if (OW_RESET_PRE == 64)
	#define OW_RESET_DELAY()		OW_DELAY_64(OW_RESET_DELAY_us)	//OW_RESET_DELAY() - сама команда котороая используется в коде программы для начала отсчета.
#elif (OW_RESET_PRE == 32)
	#define OW_RESET_DELAY()		OW_DELAY_32(OW_RESET_DELAY_us)
#elif (OW_RESET_PRE == 1)
	#define OW_RESET_DELAY()		OW_DELAY_1(OW_RESET_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) >= 255)		//Проверка получившего значения счетчика для выбранного предделителя
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) <= 2)
	#error "Prescaller less 2"
#endif
*/

#endif /* TIMER2CTRL_H_ */