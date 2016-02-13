/*
 * keyboard.c
 *	Обслуживание клавиатуры
 *	Здесь только обработка подавления дребезга.
 *	Все операции выполняются через вызовы функций меню MenuStep и MenuOK
 *  Так же используются функции SoundOn и SoundOff для генерации звука при нажатии
 */ 


#ifdef EXTERNAL_INTERAPTS_USE
	#include <avr/interrupt.h>
#endif

#include "keyboard.h"
#include "EERTOS.h"
#include "bits_macros.h"
#include "i2c.h"
#include "MenuControl.h"
#include "Alarm.h"
#include "Sound.h"


volatile u08 Key = 0;										//Состояние кнопок
volatile u08 PrevKey = 0;									//Прочитанное состояние клавиатуры перед защитным интервалом

#ifndef EXTERNAL_INTERAPTS_USE
	void KeyScan(void);										//функция сканирования кнопок если не используется прерывания
#endif

/************************************************************************/
/* Проверка необходимости возврата в основной режим по таймауту и		*/
/* установка интервала таймаута                                         */
/* Вызывается с параметром Minuts = 0 для проверки необходимости		*/
/* в основной режим, либо вызывается при получении какой-либо команды от*/
/* клавиатуры или приемника ИК с количеством минут таймаута				*/
/************************************************************************/
void TimeoutReturnToClock(u08 Minuts){
	static u08 CountWaitingEndSet;							//Отсчет интервала в течении которого не было нажатий кнопок или команд от ИК-пульта.
	
	if (Minuts)												//Установить счетчик таймаута
		CountWaitingEndSet = Minuts;
	else{													//Проверить необходимость возврата в основной режим
		if (CountWaitingEndSet)								//Проверка необходимости возврата в основной режим часов по таймауту
			CountWaitingEndSet--;							//Надо еще подождать
		else{
			if ((ClockStatus != csClock)					//Кроме специальных режимов
				&& 
				(ClockStatus != csAlarm) 
				&& 
				(ClockStatus != csSecond)
				&&
				(ClockStatus != csInternetErr)
				){//Возвращаемся в основной режим по истечении таймаута бездействия
				MenuIni();									//Сброс меню
				SetClockStatus(csClock, ssNone);			//Сброс режима часов
			}
		}
	}
}

/************************************************************************/
/*  Подтверждение нажатия клавиши и его обработка						*/
/*   Функция срабатывает через защитный интервал после прерывания		*/
/************************************************************************/
inline void BeginScakKeyRepeat(void){						//Начать обработку следующего нажатия
#ifdef EXTERNAL_INTERAPTS_USE
	GICR |= KEY_INT_MASK;									
#else
	SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
#endif
}

void KeyCheck(void){

	if	(
		BitIsSet(PrevKey, KEY_STEP_FLAG)					//Нужно повторное считывание порта STEP
		&&
		BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		)
		SetBit(Key, KEY_STEP_FLAG);
	if	(
		BitIsSet(PrevKey, KEY_OK_FLAG)						//Нужно повторное считывание порта OK
		&&
		BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		)
		SetBit(Key, KEY_OK_FLAG);
	PrevKey = 0;
	if (Key){												//Да, есть нажатие какой-то клавиши
		if (AlarmBeepIsSound()){							//Если сигнал будильника включен то выключить его
			SoundOff();
			SetClockStatus(csClock, csNone);				//Вернутся в нормальный режим
			Key = 0;
			BeginScakKeyRepeat();
			return;											//и выйти т.к. это нажатие выключения будильника
		}
	TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);		//Установить интервал таймаута для возврата в основной режим 
	SoundOn(SND_KEY_BEEP);									//Пискнуть кнопкой
	}
	if BitIsSet(Key, KEY_STEP_FLAG){						//Шаг
		MenuStep();
	}
	else if BitIsSet(Key, KEY_OK_FLAG){						//ОК
		MenuOK();
	}
	Key = 0;												//Все клавиши отработаны
	BeginScakKeyRepeat();
}

#ifdef EXTERNAL_INTERAPTS_USE
/************************************************************************/
/* Прерывание от кнопки STEP							                */
/************************************************************************/
ISR(KEY_STEP_INTname){
	if BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		SetBit(PrevKey, KEY_STEP_FLAG);
	ClearBit(GICR, KEY_STEP_INT);							//Выключить прерывания от уже сработавших кнопок
	SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
}

/************************************************************************/
/* Прерывание от кнопки ОК												*/
/************************************************************************/
ISR(KEY_OK_INTname){
	if BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		SetBit(PrevKey, KEY_OK_FLAG);
	ClearBit(GICR, KEY_OK_INT);								//Выключить прерывания от уже сработавших кнопок
	SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
}
#else
/************************************************************************/
/* Сканирование кнопок                                                  */
/************************************************************************/
void KeyScan(void){
	if BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		SetBit(PrevKey, KEY_STEP_FLAG);
	if BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		SetBit(PrevKey, KEY_OK_FLAG);
	if (PrevKey)											//Какая-то клавиша нажата, запускаем функцию проверки через защитынй интервал
		SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
	else													//Ничего не нажато, продолжаем сканировать
		SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
}
#endif

/************************************************************************/
/* Инициализация обработки нажатий на клавиатуре                        */
/************************************************************************/
void InitKeyboard(void){
	MenuIni();
	ClearBit(KEY_PORTIN_OK, KEY_OK_PORT);
	ClearBit(KEY_PORTIN_STEP, KEY_STEP_PORT);
#ifdef EXTERNAL_INTERAPTS_USE
	MCUCR |= (1<<ISC01) | (1<<ISC00) |						//По спадающему уровню на INTo
			 (1<<ISC11) | (1<<ISC10);						//по спадающему на INT1
	GICR |= (1<<INT0) | (1<<INT1);							//Разрешить прерывание от кнопок
#else
	SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
#endif
	PrevKey = 0;
	Key = 0;
}
