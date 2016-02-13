/*
 * Работа с будильниками
 * ver. 1.1
 */ 

#ifndef ALARM_H_
#define ALARM_H_

#include "bits_macros.h"
#include "avrlibtypes.h"
#include "Clock.h"

#define ALARM_MAX					9		//Количество будильников
#define ALARM_EVRY_WEEK_NUM			4		//Количество будильников в еженедельном режиме
#define ALARM_DURATION_MAX			31		//Максимальное количество минут срабатывания будильника
#define AlarmIsEvryWeek(Num)		(Num<ALARM_EVRY_WEEK_NUM)	//Будильник по дням недели
#define AlarmIsDate(Num)			(Num>=ALARM_EVRY_WEEK_NUM)	//Будильник по дате

struct sAlarm{
	u08 Id;								//Номер будильника
	u08 EnableDayRus;					//Разрешение срабатывания по дням недели.Биты: 0 - включение-выключение будильника, 1-пн,2-вт,3-ср,4-чт,5-пт,6-сб,7-вс.
	u08 Duration;						//Длительность звучания будильника в секундах. от 1 до 99. Причем старший бит всегда равен 0, т.к. он используется как флаг паузы в CurrentDuration
	u08 CurrentDuration;				//Оставшаяся длительность звучания после срабатывания будильника. Старший бит используется как флаг установки звука будильника пользователем на паузу. Установка на паузу не начинает отсчет длительности звучания сначала.
	struct sClockValue Clock;			//Время срабатывания
};
//TODO:Не реализован механизм установки на паузу с использованием переменной CurrentDuration

//------------- Флаги EnableDayRus
//Управление режимами срабатывания будильников
#define ALARM_ON_BIT				0												//Номер бита включения-выключения будильника в EnableDay.
#define AlarmOn(al)					SetBit((al).EnableDayRus, ALARM_ON_BIT)			//Будильник включить
#define AlarmOff(al)				ClearBit((al).EnableDayRus, ALARM_ON_BIT)		//Будильник выключить
#define AlarmIsOn(al)				BitIsSet((al).EnableDayRus, ALARM_ON_BIT)		//Будильник включен?
#define AlarmIsOff(al)				BitIsClear((al).EnableDayRus, ALARM_ON_BIT)		//Будильник выключен?
#define ALARM_START_DAY				1												//Первый день недели срабатывания будильника
//Управление будильником в режиме дней недели.
#define AlrmDyIsOn(al, dy)			BitIsSet((al).EnableDayRus, dy)					//Проверка включения в конкретный день
#define AlrmDyIsOff(al, dy)			BitIsClear((al).EnableDayRus, dy)				//Проверка выключения в конкретный день
#define AlrmDyOn(al, dy)			SetBit((al).EnableDayRus, dy)
#define AlrmDyOff(al, dy)			ClearBit((al).EnableDayRus, dy)

//------------- Время звучания будильника для Duration
#define ALARM_COUNT_DURAT_BITS		7												//Количество бит в счетчике длительности
#define ALARM_DURAT_MASK			0b01111111										//Биты длительности сигнала будильника настроенные
#define AlarmDuration(al)			((al).Duration & ALARM_DURAT_MASK)				//Настроенная длительность сигнала будильника
#define AlarmCurrentDur(al)			((al).CurrentDuration & ALARM_DURAT_MASK)		//Текущая длительность звучания будильника. Счетчик обратного отсчета. От Duration до 0
#define AlarmStartDurat(al)			do{(al).CurrentDuration = (al).Duration & ALARM_DURAT_MASK;} while (0)	//Начать отсчет длительности звучания
#define AlarmPauseDurat(al)			SetBit((al).CurrentDuration, 7)					//Отсчет длительности установить на паузу
#define AlarmIsNotPause(al)			BitIsClear((al).CurrentDuration, 7)				//Отсчет не на паузе
#define AlarmStopDurat(al)			do{(al).CurrentDuration = 0;}while(0)			//Прекратить отсчет длительности

void AlarmIni(void);
void AlarmCheck(void);						//Проверка необходимости включения будильника
u08 TestAlarmON(void);						//Проверить состояние текущего будильника 1-включен, 0-выключен
struct sAlarm *FirstAlarm(void);			//Возвращает указатель на первый будильник в массиве будильников
struct sAlarm *ElementAlarm(u08 NumAlarm);	//Возвращает указатель на будильник номер NumAlarm
void SetNextShowAlarm(void);				//Выбрать следующий будильник в качестве текущего
void SwitchAlarmStatus(void);				//Включить-выключить текущий будильник
u08 LitlNumAlarm(void);						//Проверка номера текущего будильника от 0 до 3
void ChangeCounterAlarm(void);				//Увеличивает или уменьшает время текущего счетчика текущего будильника
void AddDurationAlarm(void);				//Увеличивает длительность сигнала будильника на 1
void AlarmDaySwitch(void);					//Включить-выключить будильник в определенный день недели

#endif /* ALARM_H_ */