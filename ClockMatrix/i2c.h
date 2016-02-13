/*
 * i2c.h
 * Интерфейс к устройствам нм шине i2c:
	1. RTC - DS3231SN
	2. Датчик температуры LM75AD
 * v.1.3
 */ 

#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>

#include "bits_macros.h"
#include "avrlibtypes.h"
#include "IIC_ultimate.h"
#include "EERTOS.h"
#include "Clock.h"
#include "CalcClock.h"

//---------  Адреса устройств на шине i2c. Младший бит - это флаг чтения-записи в протоколе i2c
#define RTC_ADR					0b11010000			//Адрес RTC
#define EXTERN_TEMP_ADR			0b10011110			//Адрес LM75AD

//*********  Обслуживание i2c интерфейса RTC *********************
#define REFRESH_CLOCK		100						//период чтения значения текущего времени из микросхемы часов
#define REPEAT_WRT_RTC		25						//Период повторения записи если попытка записи не удалась

#define DisableIntRTC		ClearBit(INT_RTCreg, INT_RTC)
#define EnableIntRTC		SetBit(INT_RTCreg, INT_RTC)

//---------  Адреса регистров RTC
#define clkadrSEC		0x0						//Адрес счетчика секунд
#define clkadrMINUTE	0x1						//Минуты
#define clkadrHOUR		0x2						//часы
#define clkadrDAY		0x3						//День
#define clkadrDATA		0x4						//Дата
#define clkadrMONTH		0x5						//Месяц
#define clkadrYEAR		0x6						//Год
#define alrm1adrSEC		0x7						//Будильник 1 секунды
#define alrm1adrMINUTE	0x8						//минуты
#define alrm1adrHOURE	0x9						//часы
#define alrm1adrDATA	0xa						//день или дата
#define alrm2adrMINUTE	0xb						//Будильник 2 минуты
#define alrm2adrHOUR	0xc						//часы
#define alrm2adrDATA	0xd						//день или дата
#define ctradrCTRL		0xe						//Регистр контроля. Управление выводами и генератором
#define ctradrSTATUS	0xf						//Регистр статуса и контроля. Статус блоков RTC
#define ctradrAGGIN		0x10					//Значение коэффициента температурной коррекции
#define ctradrMSB_TPR	0x11					//Целая часть температуры в прямом дополнительном коде.
#define ctradrLSB_TPR	0x12					//Биты 7 и 8 это дробная часть температуры с точностью 0,25 градуса. Т.е. 0 - это 0b00000000, 0b01000000 - 0.25, 0b10000000 - 0.5, 0b11000000 -0.75

//---------  Режимы работы будильника
#define maBitRTC		6						//Номер бита режима срабатывания будильника в RTC - по дням недели или по датам
#define maDY_DT_MaskRTC	Bit(maBitRTC)			//Маска флага режима срабатывания будильника в RTC
#define AlrmIsDYRTC(Mc)	BitIsSet(Mc, maBitRTC)
//---------  Биты контрольного регистра
#define DisableOSC_RTC	7						//Запуск-остановка генератора RTC. 1 - остановить генератор
#define EnableWave_RTC	6						//Разрешить выход генератора на вывод SQW
#define StartTXO_RTC	5						//запустить измерение температуры в ручную
#define RS2_RTC			4						//частота сигнала на выводе SQW. По умолчанию 8.192 kHz
#define RS1_RTC			3
#define Int_Cntrl_RTC	2						//Режим вывода INT или SQW. По умолчанию INT (бит взведен)
#define EnableAlrm2RTC	1						//Номер бита разрешения прерывание от будильника 2
#define EnableAlrm1RTC	0						//Номер бита разрешения прерывание от будильника 1
#define Alrm1IsOnRTC(Mc) BitIsSet(Mc, EnableAlrm1RTC)
#define Alrm2IsOnRTC(Mc) BitIsSet(Mc, EnableAlrm2RTC)
//--------- Биты регистра статуса RTC
#define RTC_OSF_FLAG	7						//Генератор отключался, а значит данные недостоверны
#define RTC_TEMP_BUSY	2						//RTC занят расчетом температуры
#define RTC_32KHZ_ENBL	3						//Разрешить работу генератора 32 кГц

extern volatile u08 StatusModeRTC;				//Текущий режим работы с RTC микросхемой. Младшие 5 бит это текущий адрес записи в RTC
#define WR_MODE_BIT		7						//Бит режима чтения-записи RTC
#define ModeRTCIsRead	BitIsClear(StatusModeRTC, WR_MODE_BIT)
#define ModeRTCIsWrite	BitIsSet(StatusModeRTC, WR_MODE_BIT)
#define SetModeRTCRead	ClearBit(StatusModeRTC, WR_MODE_BIT)
#define SetModeRTCWrite	SetBit(StatusModeRTC, WR_MODE_BIT)
#define CALC_ADD_WR_BIT	6						//Бит направления счета: Add или Dec. После удачной записи в RTC режим всегда сбрасывается в Add (прибавление 1 к текущьему настариваемому счетчику)
#define CalcIsAdd		BitIsSet(StatusModeRTC, CALC_ADD_WR_BIT)
#define CalcIsDec		BitIsClear(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcAdd		SetBit(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcDec		ClearBit(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcDirect	SetBit(StatusModeRTC, CALC_ADD_WR_BIT)

#define SetWrtAdrRTC(Adr)	do {StatusModeRTC = ((StatusModeRTC & 0b11100000) | (Adr & 0b00011111));} while (0); //Установить адрес записи в RTC
#define GetWrtAdrRTC	(StatusModeRTC & 0b00011111)

typedef	void (*SECOND_RTC)(void);				//Тип функции вызываемый каждую секунду от срабатывания счетчика RTC.

void Init_i2cRTC(void);							//Инициализация RTC
void SetSecondFunc(SECOND_RTC Func);			//Определить функцию вызываемую каждую секунду
SECOND_RTC GetSecondFunc(void);					//Получить ссылку на функцию вызываемую каждую секунду
void WriteToRTC(void);							//Начать запись в RTC
void Set0SecundFunc(SECOND_RTC Func);			//Определить функцию вызываемую каждую нулевую секунду для будильника
u08 i2c_RTC_DirectWrt(void);					//Прямая запись структуры Watch в RTC, возвращает 1 если запись успешно стартовала

//extern volatile u08 TemprCurrent;				//Текущее значение температуры в дополнительном коде. Хотя RTC и измеряет температуру с точностью до 0.25 градусов, используется только целое значение
extern volatile u08 OneSecond;					//Меняет значение 0-1 каждую секунду

//*********  Обслуживание i2c интерфейса датчика температуры *********************
#define REFRESH_EXT_TMPR	105					//период повторения чтения значения температуры из внешнего датчика температуры. Выбрано 105 мс чтобы разминутся с чтением из RTC которое читается с частотой 105 мс
#define REPEAT_EXT_TMPR		25					//Период повторения записи если попытка записи не удалась
#define i2c_ExtTmprBuffer	2					//Количество читаемых байт из датчиа температуры за один сеанс

//---------  Адреса регистров датчика температуры
#define extmpradrTEMP		0x0					//Температура
#define extmpradrCONF		0x1					//Конфигурация
#define extmpradrTHYST		0x2					//Температура гистерезиса для ноги OS
#define extmpradrTOS		0x3					//Температура переключения ноги OS
//---------  Биты конфигурационного регистра
#define SleepExtTmpr		0					//Прекратить измерение температуры, выдается последнее измеренное значение
#define OSPinModeExtTmpr	1					//Режим работы ноги OS. 0 - обычное управление, 1 - выдача в виде импульса.
#define OSPolModeExtTmpr	2					//0 - Превышение заданной температуры приводит к выдаче низкого уровня на ноге OS, 1 - активный высокий уровень
#define OSQueModeExtTmpr0	3					//Количество превышений температуры для ноги OS
#define OSQueModeExtTmpr1	4

void Init_i2cExtTmpr(void);						//Инициализация наружного датчика температуры
void i2c_ExtTmpr_Read(void);					//Запуск получения результатов измерения

#endif /* I2C_H_ */