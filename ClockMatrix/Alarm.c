/*
 * Работа с будильниками
 * ver. 1.1
 */ 

#include <avr\interrupt.h>
#include "Alarm.h"
#include "CalcClock.h"
#include "Sound.h"
#include "eeprom.h"
#include "esp8266hal.h"

struct sAlarm Alarms[ALARM_MAX];											//Будильники

/************************************************************************/
/* Проверка будильников на срабатывание.								*/ 
/*  Должна вызываться каждую нулевую секунду в минуте                   */
/************************************************************************/
#define AlarmCheckEqu()	((Alarms[i].Clock.Hour == Watch.Hour) && (Alarms[i].Clock.Minute == Watch.Minute) && (Watch.Second == 0)) //Проверка совпадения указанного будильника и текущего времени
void AlarmCheck(void){
	u08 i;

	if (ClockStatus == csAlarm){											//Режим сработавшего будильника, надо проверять необходимость отключения звука будильника
		for (i=0; i<ALARM_MAX; i++){										//Проверка необходимости выключения будильника для всех будильников
			if AlarmIsNotPause(Alarms[i]){									//Звук будильника не на паузе
				if AlarmCurrentDur(Alarms[i]){								//Текущая длительность звучания не 0, а значит этот будильник звучит в данный момент
					Alarms[i].CurrentDuration--;							//Вычитаем 1 минуту
					if (AlarmCurrentDur(Alarms[i]) == 0){					//Весь интервал звучания выбран, выключаем будильник
						SoundOff();											//Выключить звук
						SetClockStatus(csClock, csNone);					//Вернутся в нормальный режим
					}
				}
			}
			else{															//Звук будильника на паузе, начать или продолжить отсчет паузы
																			//Еще не реализовано
			}
		}
	}
	
	for(i=0;i<ALARM_MAX; i++){												//Проверка необходимости включения сигнала будильника для всех будильников
		if (AlarmIsOn(Alarms[i]) && (AlarmCurrentDur(Alarms[i]) == 0)){		//Этот будильник включен? и еще не срабатывал
			if AlarmCheckEqu(){												//Время совпадает
				if AlarmIsEvryWeek(i){										//Это будильник по дням недели
					if AlrmDyIsOff(Alarms[i], what_day(Watch.Date, Watch.Month, Watch.Year)){	//и день не совпадает
						continue;											//Будильник не сработал	
					}
				}
				else if (AlarmIsDate(i) &&
						((Alarms[i].Clock.Date != Watch.Date) ||
						(Alarms[i].Clock.Month != Watch.Month))){				//Будильник по дате и дата не та
					continue;
				}
				SetClockStatus(csAlarm, csNone);								//Отобразить срабатывание будильника
				AlarmStartDurat(Alarms[i]);										//Начать отсчет длительности сигнала будильника
				SoundOn(SND_ALARM_BEEP);										//Звук включить
			}
		}
	}

	if (Watch.Minute == 0){													//Пищать каждый час
		i = HourToInt(Watch.Hour);											//Преобразовать в десятичное число
		if ((i >= EACH_HOUR_START) && (i<=EACH_HOUR_STOP))					//Куранты только в указанный промежуток времени
			SoundOn(SND_EACH_HOUR);
	}
}

/************************************************************************/
/*  Включить-выключить будильник в текущий день                         */
/************************************************************************/
void AlarmDaySwitch(void){
	if (AlrmDyIsOn(*CurrentShowAlarm, CurrentAlarmDayShow))
		AlrmDyOff(*CurrentShowAlarm, CurrentAlarmDayShow);
	else
		AlrmDyOn(*CurrentShowAlarm, CurrentAlarmDayShow);
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Добавить 1 к длительности звучания текущего будильника               */
/************************************************************************/
void AddDurationAlarm(void){
	if ((*CurrentShowAlarm).Duration == ALARM_DURATION_MAX)
		(*CurrentShowAlarm).Duration = 1;
	else
		(*CurrentShowAlarm).Duration += 1;
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Увеличивает текущий счетчик текущего будильника                      */
/************************************************************************/
void ChangeCounterAlarm(void){
	volatile u08 *Tmp = &((*CurrentCount).Minute);
	
	switch (SetStatus){
		case ssMinute:
			Tmp = &((*CurrentCount).Minute);
			break;
		case ssHour:
			Tmp = &((*CurrentCount).Hour);
			break;
		case ssDate:
			Tmp = &((*CurrentCount).Date);
			break;
		case ssMonth:
			Tmp = &((*CurrentCount).Month);
			break;
		case ssYear:
			Tmp = &((*CurrentCount).Year);
			break;
		default:
			break;
	}
	*Tmp = CalcIsAdd?AddClock(CurrentCount, SetStatus):DecClock(CurrentCount, SetStatus);
	SetCalcAdd;													//По умолчанию всегда добавление 1
	AlarmStopDurat(*CurrentShowAlarm);							//Текущее срабатывание сбросить
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Возвращает 1 если текущий номер будильника							*/
/* от 0 до ALARM_EVRY_WEEK_NUM (еженедельное срабатывание),				*/
/* или 0 если от ALARM_EVRY_WEEK_NUM до ALARM_MAX (по датам)			*/
/************************************************************************/
u08 LitlNumAlarm(void){
	return (CurrentShowAlarm->Id <= ALARM_EVRY_WEEK_NUM-1)?1:0;
}

/************************************************************************/
/* Возвращает указатель на первый будильник в массиве будильников		*/
/************************************************************************/
struct sAlarm *FirstAlarm(void){
	return 	&Alarms[0];
}

/************************************************************************/
/* Возвращает указатель на будильник номер NumAlarm                     */
/************************************************************************/
struct sAlarm *ElementAlarm(u08 NumAlarm){
	if (NumAlarm<ALARM_MAX)
		return &Alarms[NumAlarm];
	else
		return &Alarms[0];
}


/************************************************************************/
/* Выбор следующего будильника                                          */
/************************************************************************/
void SetNextShowAlarm(void){
	if (CurrentShowAlarm->Id == ALARM_MAX-1)
		CurrentShowAlarm = &Alarms[0];
	else
		CurrentShowAlarm = &Alarms[CurrentShowAlarm->Id+1];
	CurrentCount = &(CurrentShowAlarm->Clock);					//Текущий счетчик времени будет счетчик будильника
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Включение - выключение текущего будильника                           */
/************************************************************************/
void SwitchAlarmStatus(void){
	if AlarmIsOn(*CurrentShowAlarm){
		AlarmOff(*CurrentShowAlarm);
	}
	else
		AlarmOn(*CurrentShowAlarm);
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Проверить состояние будильника: выключен или выключен                */
/************************************************************************/
u08 TestAlarmON(void){
	return AlarmIsOn(*CurrentShowAlarm)?1:0;
}

void AlarmIni(void){
	u08 i;

	for(i=0; i<ALARM_MAX; i++){								//Настройка будильников по умолчанию
		Alarms[i].Id = i;
		if (EeprmReadAlarm(&Alarms[i]) == 0){				//Не удалось прочитать состояние будильника из eeprom, настраивается по умолчанию
			AlarmOff(Alarms[i]);							//Будильник выключить
			Alarms[i].Duration = 1;							//Длительность сигнала 1 минута
			if (i>=ALARM_EVRY_WEEK_NUM){					//Для будильников по датам инициализировать правильные даты
				Alarms[i].Clock.Date = 0x01;
				Alarms[i].Clock.Month = 0x01;
				Alarms[i].Clock.Year = 0x15;				//Хотя год и игнорируется, все равно его инициализируем
			}
			AlarmStopDurat(Alarms[i]);						//Текущее срабатывание сбросить
		}
	}
	CurrentShowAlarm = &Alarms[0];
}
