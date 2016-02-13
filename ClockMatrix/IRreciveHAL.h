/*
 * IRrecive.h
 * ver 1.2
 */ 


#ifndef IRRECIVEHAL_H_
#define IRRECIVEHAL_H_

#include <avr/interrupt.h>
#include <stddef.h>
#include "bits_macros.h"
#include "avrlibtypes.h"
#include "DisplayHAL.h"				//Отсчет периодов импульсов с помощью прерываний от тактового генератора микросхемы RTC - 32 768 Гц (переменная PeriodIR)
#include "IRrecive.h"
#include "Clock.h"

void IRRecivHALInit(void);			//Инициализация приемника ИК

u08 ConvertIRCodeToDec(u08 Code);	//Преобразование кода принятой клавиши в цифру, возвращает 10 если код не соответствует цифре

#endif /* IRRECIVEHAL_H_ */