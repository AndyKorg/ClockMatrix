/*
 * Подробно в заголовочном файле IRrecive.h
 * ver 1.4
 */ 

#include <stddef.h>
#include "Alarm.h"
#include "i2c.h"
#include "IRrecive.h"
#include "MenuControl.h"
#include "Sound.h"
#include "EERTOS.h"
#include "Display.h"
#include "keyboard.h"
#include "sensors.h"

#ifdef IR_SAMSUNG_ONLY
	#include "IRreciveHAL.h"
#else
	#include "IRMultiHAL.h"
#endif

#define IR_INDICATOR_X	0							//Координата точки индицирования приема пакета от ИК приемника
#define IR_INDICATOR_Y	7
#define IR_DURAT_BLINK	200							//Длительность моргания индикатора получения ИК пакета
#define IR_COUNT_BLINK	((u08) 1000/IR_DURAT_BLINK)	//Количество морганий индикатора за 1000 мс

volatile u16 PeriodIR;								//Отсчитанный период между импульсами ИК приемника, используется в декодировании посылок с ИК-пульта
u32 IRcmdList[IR_NUM_COMMAND];						//Коды ИК пульта

VOID_PTR_VOID IRPeriodEvent = NULL;					//Ссылка ну функцию обработки события PERIOD_INT_IR
RECIV_IR_PTR IRReciveReady = NULL;					//Должна быть указана функция для обработки принятых пакетов. Т.к. фукнция вызывается в прерывании, то она должна быть достаточно быстрой

void IRReciveRdy(u08 AdresIR, u32 CommandIR);
/************************************************************************/
/* Включение штатной обработки команд от ИК пульта.						*/
/************************************************************************/
void IRReciveRdyOn(void){
	IRReciveReady = IRReciveRdy;
}
/************************************************************************/
/* Замещает цифру в формате BDC либо в старшем нибле либо в младшем     */
/************************************************************************/
u08 AddDigitNibble(u08 Value, u08 Digit, u08 HighNibbl){
	if (HighNibbl)
		return ((Value & 0xf0) | (Digit<<4));
	else
		return ((Value & 0x0f) | Digit);
}

/************************************************************************/
/* Индикация приема пакета от ИК приемника                              */
/************************************************************************/
void IRIndicatorBlink(void){
	static u08 Count = IR_COUNT_BLINK;
	plot(IR_INDICATOR_X, IR_INDICATOR_Y, DotIsSet(IR_INDICATOR_X, IR_INDICATOR_Y));
	if (Count){
		Count--;
		SetTimerTask(IRIndicatorBlink, IR_DURAT_BLINK);
	}
}

/************************************************************************/
/* Старт индикации приема пакета от ИК пульта							*/
/************************************************************************/
void IRStartIndicator(void){
	plot(IR_INDICATOR_X, IR_INDICATOR_Y, 1);			//Включить индикатор приема пакета от ИК
	SetTimerTask(IRIndicatorBlink, IR_DURAT_BLINK);
}

/************************************************************************/
/* Получить индекс команды по ее коду                                   */
/************************************************************************/
#define IR_CMD_UNKNOWN	0xff							//Команда не опознана. Используется код 0xff поскольку такая хеш функция кода пульта невозможна.
inline u08 DecodeCmd(u32 Cmd){
	for(u08 i=0; i<IR_NUM_COMMAND; i++)
		if (IRcmdList[i] == Cmd)
			return i;
	return IR_CMD_UNKNOWN;
}

/************************************************************************/
/* Разрешение работы приемника ИК после защитного периода               */
/************************************************************************/
void IRStartFromDelay(void){
	StartIRCounting();									//Обработка команды закончена можно принимать следующую
}

/************************************************************************/
/* Обработка команд полученных от ИК пульта по протоколу samsung        */
/************************************************************************/
void IRReciveRdy(u08 AdresIR, u32 CommandIR){
		
	if (AdresIR == REMOT_TV_ADR){							//Команда с пульта samsug
		u08 IrCmd = DecodeCmd(CommandIR);
		if	(
			(AlarmBeepIsSound() == 0)						//Пищать только если звук будильника еще не включен
			&&
			(IrCmd != IR_PLAY_START)						//и не запуск звука
#ifndef IR_SAMSUNG_ONLY										//Если используется произвольный ИК пульт, то пищать только для опознанных кодов. Сделан для того что бы не пищало на всякий мусор
			&&
			(IrCmd != IR_CMD_UNKNOWN)
#endif		
			)
			SoundOn(SND_KEY_BEEP);							//Пискнуть если разрешено
		if (IrCmd != IR_CMD_UNKNOWN){						
			IRStartIndicator();								//Команда опознана вывести точку
			if (ClockStatus == csAlarm){
				SetClockStatus(csClock, ssNone);			//Выключить режим будильника если включен
				SoundOff();									//Звук выключить
			}
			else{											//Обычный режим, просто обработать команду
				TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);//Взвести счетчик таймаута возврата в основной режим
				switch (IrCmd){
					case IR_PLAY_START:
						if (AlarmBeepIsSound() == 0)			//Воспроизвести мелодию будильника, если она еще не звучит
							SoundOn(SND_ALARM_BEEP);
						break;
					case IR_PLAY_STOP:
						if (AlarmBeepIsSound() == 1)			//Остановить воспроизведение, если оно идет
							SoundOff();
						break;
					case IR_OK:									//Меню ОК
						MenuOK();
						break;
					case IR_STEP:								//Меню Step
						MenuStep();
						break;
					case IR_INC:								//Увеличить номер канала - здесь увеличить число в счетчике
					case IR_DEC:								//Обратное действие
						if (
							 (
								(ClockStatus == csSet) ||		//Или в часах
								(ClockStatus == csAlarmSet)		//или в будильниках
							 )
							&& (SetStatus != ssNone)			//И должен быть выбран конкретный счетчик
						   ){			
							if (IrCmd == IR_INC)
								SetCalcAdd;
							else
								SetCalcDec;
							if (ClockStatus == csSet)			//Счетчики часов пишутся в микросхему RTC
								WriteToRTC();
							else
								ChangeCounterAlarm();			//Изменить счетчик будильника
						}
						break;
					default:
						break;
				}//switch
			}
		}
		SetTimerTask(IRStartFromDelay, SCAN_PERIOD_KEY);
	}//if (AdresIR == REMOT_TV_ADR)
#ifdef IR_SAMSUNG_ONLY
	else{												//Какие-то другие данные, не от пульта ТВ
		SetSensor(AdresIR, AdresIR, CommandIR);			//Пока считаем что это данные от внешнего датчика температуры. Поскольку байт AdresIR получаемый от датчика содержит и статус и адрес, но в разных ниблах, то он просто передается два раза.
	}
#else
	#warning "IR reciver for sensors is off"
#endif
}

void IRReciverInit(void){
	IRRecivHALInit();
	IRReciveReady = IRReciveRdy;						//определить функцию обработки принятых данных ИК
}
