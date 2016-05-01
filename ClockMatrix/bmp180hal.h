/*
 * Обслуживание датчика давления BNP180
 *
 */ 


#ifndef BMP180HAL_H_
#define BMP180HAL_H_

#include "avrlibtypes.h"
#include "eeprom.h"

//Регистры
#define BMP180_CONTROL_REG			0xf4		//Регистр управления
#define BMP180_RESET_REG			0xe0		//Регистр программного сброса.
#define BMP180_CHIP_ID_REG			0xd0		//Регистр id чипа
#define BMP180_DATA_REG_START		0xf6		//Начальный адрес регистра данных
//Адреса калибровочных коэффициентов, _S_ - со знаком, _U_ - Без знака
#define BMP180_COFF_COUNT			22			//Количество байт для калибровочных коэффициентов
#define BMP180_COEFF_ADR_START		0xAA		//Стартовый адрес блока регистров поправочных коэффициентов
#define BMP180_COEFFCNT_S_AC1		0			//Адрес первого байта коэффициента AC1, все коэффициенты 16-битные, первым идет старший байт. BMP180_COEFFCNT_S_AC1+1 младший байт
#define BMP180_COEFFCNT_S_AC2		2
#define BMP180_COEFFCNT_S_AC3		4
#define BMP180_COEFFCNT_U_AC4		6
#define BMP180_COEFFCNT_U_AC5		8
#define BMP180_COEFFCNT_U_AC6		10
#define BMP180_COEFFCNT_S_B1		12
#define BMP180_COEFFCNT_S_B2		14
#define BMP180_COEFFCNT_S_MB		16
#define BMP180_COEFFCNT_S_MC		18
#define BMP180_COEFFCNT_S_MD		20
//Команды
#define BMP180_SOFT_RESET			0xb6		//Команда сброса для регистра BMP180_RESET_REG
//Команды запуска измеренией для регистра BMP180_CONTROL_REG
#define BMP180_TEPERATURE_START		0x2e		//Начать измерение температуры. время измерения 4.5 мс
#define BMP180_PRESRE_LOW_START		0x34		//Давление в режиме oss=0 (самое низкое потребление), время измерения 4.5 мс
#define BMP180_PRESRE_NORMAL_START	0x74		//в режиме oss = 1 (стандартный режим), время измерения 7.5 мс
#define BMP180_PRESRE_HI_RES_STAT	0xb4		//высокое разрешение - oss=2, 13,5 мс
#define BMP180_PRESRE_UL_RES_START 	0xf4		//сверхвысокое разрешение, oss=3, 25.5 mc
//длительность измерений, мс
#define BMP180_TEMPERATURE_TIME		5			//4.5 мс на самом деле
#define BMP180_PRESRE_LOW_TIME		5			//время измерения 4.5 мс
#define BMP180_PRESRE_NORMAL_TIME	8			//в режиме oss = 1 (стандартный режим), время измерения 7.5 мс
#define BMP180_PRESRE_HI_RES_TIME	14			//высокое разрешение - oss=2, 13,5 мс
#define BMP180_PRESRE_UL_RES_TIME 	26			//сверхвысокое разрешение, oss=3, 25.5 mc

#define BMP180_CMD_LEN				1			//Длина команды 1 байт
#define	BMP180_COFF_LEN				2			//Длина данных калибровочных коэфф.
#define	BMP180_TEMPER_LEN			2			//Длина данных температуры
#define	BMP180_PRESS_LEN			3			//Длина данных давления

#define BMP180_REPEAT_TIME_MS		50			//Период повторения запуска измерений если шина I2C занята

//Используемое разрешение
#define BMP180_PRESSURE_START		BMP180_PRESRE_NORMAL_START	//<<<<<------- изменить если требуется другая точность
//Определение времени измерения
#if (BMP180_PRESSURE_START == BMP180_PRESRE_LOW_START)
	#define BMP180_OSS				0
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_LOW_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_NORMAL_START)
	#define BMP180_OSS				1
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_NORMAL_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_HI_RES_STAT)
	#define BMP180_OSS				2
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_HI_RES_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_UL_RES_START)
	#define BMP180_OSS				3
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_UL_RES_TIME
#else
	#error "Undefine BMP180_PRESSURE_START"
#endif

#define  BMP180_INVALID_DATA		0x1fff		//Ошибка получения давления и температуры
typedef void (*PRESS_AND_TEMPER)(u16*, u16*);	//Функция вызываемая по окончании  измерения или таймаута

void StartMeasuringBMP180(void);

PRESS_AND_TEMPER PressureRead;

#endif /* BMP180HAL_H_ */