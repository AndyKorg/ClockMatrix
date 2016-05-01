/*
 * Чтение и запись информации микросхемы RTC
 */ 

#include "i2c.h"
#include "sensors.h"
#include "ds18b20.h"
#include "bmp180hal.h"
#include "esp8266hal.h"

//volatile u08 TemprCurrent;					//Текущее значение температуры в дополнительном коде. Хотя RTC и измеряет температуру с точностью до 0.25 градусов, используется только целое значение
volatile u08 StatusModeRTC;						//Текущий режим работы с RTC микросхемой. Младшие 5 бит это текущий адрес записи в RTC
volatile u08 OneSecond;							//Меняет значение 0-1 каждую секунду

void i2c_RTC_Exchange(void);					//Обмен с микросхемой RTC
SECOND_RTC	GoSecond = Idle,					//Функция вызываемая каждую секунду из прерывания RTC
			Go0Second = Idle;					//Функция вызываемая каждую нулевую секунду в минуте для проверки срабатывания будильника

/************************************************************************/
/* Запуск записи в микросхему RTC, адрес записи определяется				*/
/* переменной StatusModeRTC, что писать см. в функции i2c_RTC_Exchange  */
/************************************************************************/
void WriteToRTC(void){
	SetModeRTCWrite;
	if (Refresh != NULL)
		SetTimerTask(Refresh, REFRESH_CLOCK*2);//Обновить экран 
}

/************************************************************************/
/* Определить функцию вызываемую каждую секунду по прерыванию от RTC    */
/************************************************************************/
void SetSecondFunc(SECOND_RTC Func){
	GoSecond = Func;
}
/************************************************************************/
/* Возвращает ссылку на функцию вызываемую каждую секунду по прерыванию */
/************************************************************************/
SECOND_RTC GetSecondFunc(void){
	return GoSecond;
}

/************************************************************************/
/* Определить функцию вызываемую каждую нулевую секунду					*/
/************************************************************************/
void Set0SecundFunc(SECOND_RTC Func){
	Go0Second = Func;
}

/************************************************************************/
/* Прерыавние от микросхемы RTC. Происходит каждую секунду              */
/************************************************************************/
ISR(INT_NAME_RTC){
#ifndef INT_SENCE_RTC
	if (BitIsClear(INT_RTCreg, INT_RTC)) return;					//Если прерывание не от RTC то выход.
#endif	
	OneSecond = (OneSecond)?0:1;
	if (GoSecond != Idle)
		(GoSecond)();
	SetTask(espWatchTx);											//Передать при первой возможности время в esp
	if (Watch.Second == 0){											//Срабатывание каждую минуту
		if (Go0Second != Idle)										//Есть функция проверки будильника?
			(Go0Second)();
		if ((Watch.Minute & 0xf) == 0){								//Десять минут прошло
			SetTask(i2c_ExtTmpr_Read);								//Прочитать внешний датчик температуры
			SetTask(StartMeasureDS18);								//Прочитать данные с датчика 1-Ware
			SetTask(StartMeasuringBMP180);
		}
		for(u08 i=0; i<= SENSOR_MAX; i++)							//Уменьшить счетчик периода для всех датчиков
			if (SensorFromIdx(i).SleepPeriod)
				SensorFromIdx(i).SleepPeriod--;
	}
}

/************************************************************************/
/* Обмен данными с RTC                                                  */

/************************************************************************/
/* Операция удачно завершена                                            */
/************************************************************************/
void i2c_OK_RTCFunc(void){
	if (ModeRTCIsWrite){											//Если запись то сбрасываем режим записи, поскольку успешно записано
		SetModeRTCRead;
		SetCalcAdd;													//Режим прибавления 1 к счетчикам по умолчанию
	}
	else{															//Успешно прочитано пишем в нужную структуру
		Watch.Second = i2c_Buffer[clkadrSEC];
		Watch.Minute = i2c_Buffer[clkadrMINUTE];
		Watch.Mode = ClrModeBit(Watch.Mode);
		Watch.Mode |= i2c_Buffer[clkadrHOUR] & (mcModeHourMask | mcAM_PM_Mask);	//Определяется режим часов и текущий статус AM/PM
		Watch.Hour =  i2c_Buffer[clkadrHOUR] & ~(ModeIs24(Watch.Mode)? mcModeHourMask : (mcModeHourMask | mcAM_PM_Mask));
		Watch.Date = i2c_Buffer[clkadrDATA];
		Watch.Month = i2c_Buffer[clkadrMONTH] & 0b00011111;			//7-1 бит это век, всегда сбрасывается
		Watch.Year = i2c_Buffer[clkadrYEAR];
		//Температура
//		if BitIsClear(i2c_Buffer[ctradrSTATUS], RTC_TEMP_BUSY)		//Температура готова
//			TemprCurrent = i2c_Buffer[ctradrMSB_TPR];
	}
	i2c_Do &= i2c_Free;												//Освобождаем шину
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);					//Повторить запуск чтения состояния микросхемы часов
}

/************************************************************************/
/* Ошибка обмена			                                            */
/************************************************************************/
void i2c_Err_RTCFunc(void){
	i2c_Do &= i2c_Free;												//Освобождаем шину
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);					//Пробуем повторить операцию
}

void i2c_RTC_Exchange(void){
	u08 i;

	if (i2c_Do & i2c_Busy){											//Шина занята, попытаемся позже
		SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);
		return;
	}
	
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;													//Писать в буфер с нуля
	i2c_Do &= ~i2c_type_msk;										//Сброс режима
	if (ModeRTCIsWrite){											//------------------ Режим записи
		if ((ClockStatus == csSet)									//Для записи дат в режиме установки часов особый порядок
			&&
				(
				(SetStatus == ssDate) || (SetStatus == ssMonth) ||	//Дата записывается одним пакетом - дата, месяц, год
				(SetStatus == ssYear)
				)
			){
			i2c_ByteCount = 5;										//Пишем сразу четыре числа
			i2c_Buffer[0] = clkadrDAY;								//начиная с дня недели
			i = CalcIsAdd ? AddClock(CurrentCount, SetStatus) : DecClock(CurrentCount, SetStatus);//Рассчитывается следующее значение части даты
			i2c_Buffer[2] = (SetStatus == ssDate)? i : Watch.Date;
			i2c_Buffer[3] = (SetStatus == ssMonth)? i : Watch.Month;
			i2c_Buffer[4] = (SetStatus == ssYear)? i : Watch.Year;
			i2c_Buffer[1] = WeekRTC(what_day(i2c_Buffer[2], i2c_Buffer[3], i2c_Buffer[4]));//День недели всегда рассчитывается. 1 - это воскресенье в микросхеме RTC, а функция what_day возвращает 7-вс.
		}
		else if (GetWrtAdrRTC == ctradrCTRL){						//------------------ Запись регистра контроля
			i2c_ByteCount = 2;										//Сколько передавать
			i2c_Buffer[0] = ctradrCTRL;								//Адрес куда писать в RTC
			ClearBit(i2c_Buffer[1], DisableOSC_RTC);				//Генератор включен
			SetBit(i2c_Buffer[1], EnableWave_RTC);					//Включить вывод SQW
			ClearBit(i2c_Buffer[1], StartTXO_RTC);					//Измеритель температуры
			ClearBit(i2c_Buffer[1], RS1_RTC);						//Частота генератора 1 Гц
			ClearBit(i2c_Buffer[1], RS2_RTC);
			ClearBit(i2c_Buffer[1], Int_Cntrl_RTC);					//Разрешить работу вывода INT в режиме вывода генератора
		}
		else if (GetWrtAdrRTC == ctradrSTATUS){						//------------------ Запись регистра состояния
			i2c_ByteCount = 2;										//Сколько передавать
			i2c_Buffer[0] = ctradrSTATUS;							//Адрес куда писать в RTC
			SetBit(i2c_Buffer[1], RTC_32KHZ_ENBL);					//Включить генератор 32 кГц
			ClearBit(i2c_Buffer[1], RTC_OSF_FLAG);					//Генератор был включен, а значит данные будут верными после установки
		}
		else{														//------------------ Простая запись счетчиков часов, минут, секунд
			i2c_ByteCount = 2;										//Сколько передавать
			i2c_Buffer[0] = GetWrtAdrRTC;							//Адрес куда писать в RTC
			i2c_Buffer[1] = CalcIsAdd ? AddClock(CurrentCount, SetStatus) : DecClock(CurrentCount, SetStatus);	//Рассчитывается следующее значение
		}
		i2c_Do |= i2c_sawp;											//Просто запись
	}
	else{															//------------------ Чтение RTC
		i2c_ByteCount = i2c_MaxBuffer;								//количество читаемых байт
		i2c_PageAddrCount = 1;
		i2c_PageAddrIndex = 0;
		i2c_PageAddress[0] = 0;
		i2c_Do |= i2c_sawsarp;										//Режим чтение данных.
	}
	MasterOutFunc = &i2c_OK_RTCFunc;
	ErrorOutFunc = &i2c_Err_RTCFunc;
	
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //Пуск операции
	i2c_Do |= i2c_Busy;
}

/************************************************************************/
/* Прямая запись структуры Watch в RTC                                  */
/************************************************************************/
u08 i2c_RTC_DirectWrt(void){
	if (i2c_Do & i2c_Busy){											//Шина занята
		return 0;
	}
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;													//Писать в буфер с нуля
	i2c_Do &= ~i2c_type_msk;										//Сброс режима
	i2c_ByteCount = 8;												//Пишем сразу все счетчики
	i2c_Buffer[0] = clkadrSEC;										//начиная с секунд
	i2c_Buffer[1] = Watch.Second;
	i2c_Buffer[2] = Watch.Minute;
	i2c_Buffer[3] = Watch.Hour;										//Всегда 24-часовой формат времени!
	i2c_Buffer[4] = WeekRTC(what_day(Watch.Date, Watch.Month, Watch.Year));
	i2c_Buffer[5] = Watch.Date;
	i2c_Buffer[6] = Watch.Month;
	i2c_Buffer[7] = Watch.Year;
	i2c_Do |= i2c_sawp;												//Просто запись

	MasterOutFunc = &i2c_OK_RTCFunc;
	ErrorOutFunc = &i2c_Err_RTCFunc;

	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //Пуск операции
	i2c_Do |= i2c_Busy;
	return 1;
}

/************************************************************************/
/* Чтение регистра статуса перед запуском								*/
/************************************************************************/
void i2c_RTC_StatusReadOk(void){
	if (BitIsSet(i2c_Buffer[ctradrSTATUS], RTC_OSF_FLAG))		//Была остановка генератора
		RTC_ValidClear();										//Значения счетчиков не корректны
	i2c_Do &= i2c_Free;											//Освобождаем шину
}
void i2c_RTC_StatusReadErr(void){
	RTC_ValidClear();											//Значения счетчиков не корректны
	i2c_Do &= i2c_Free;											//Освобождаем шину
}

/************************************************************************/
/* Запуск обмена с RTC                                                  */
/************************************************************************/
void Init_i2cRTC(void){
	//Чтение регистра статуса микросхемы RTC
	while(i2c_Do & i2c_Busy);									//Шина занята, ждем освобождения
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;												//Писать в буфер с нуля
	i2c_Do &= ~i2c_type_msk;									//Сброс режима
	i2c_ByteCount = 1;											//количество читаемых байт
	i2c_PageAddrCount = 1;
	i2c_PageAddrIndex = 0;
	i2c_PageAddress[0] = ctradrSTATUS;							//Читаем из регистра статуса
	i2c_Do |= i2c_sawsarp;										//Режим чтение данных.
	MasterOutFunc = &i2c_RTC_StatusReadOk;
	ErrorOutFunc = &i2c_RTC_StatusReadErr;
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;  //Пуск операции
	i2c_Do |= i2c_Busy;
	while(i2c_Do & i2c_Busy);									//Ожидается результат чтения
	
	SetWrtAdrRTC(ctradrCTRL);									//Настройка RTC. Запись в регистр контроля
	SetModeRTCWrite;
	i2c_RTC_Exchange();											//Запись настроек в RTC
	while(ModeRTCIsWrite);										//Ждем окончания записи в RTC
	SetWrtAdrRTC(ctradrSTATUS);									//Настройка RTC. Запись в регистр статуса
	SetModeRTCWrite;
	i2c_RTC_Exchange();											//Запись настроек в RTC
	while(ModeRTCIsWrite);										//Ждем окончания записи в RTC
	Watch.Second = 0;											//Обнуляются счетчики т.к. возможна ситуация когда произошло односекундное прерывание, а счетчики еще не прочитаны из микросхемы RTC
	Watch.Minute = 0;
#ifdef INT_SENCE_RTC											//Работа с прерыванием INTX
	MCUCR |= INT_SENCE_RTC;
#else
	SetBit(PCICR, INT_RTCgrp);
#endif
	SetBit(INT_RTCreg, INT_RTC);								//Запуск прерывания от RTC
	SetModeRTCRead;												//Режим чтения RTC
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);				//Запуск чтения состояния микросхемы часов
}
