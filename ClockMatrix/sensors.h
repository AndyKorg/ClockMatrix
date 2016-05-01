/*
 * Обслуживание внешних датчиков
 * температуры, давления и т.п.
 * Структура описывающая датчики инициализируется следующим образом:
 * - имя датчика записывается как "дN" где N порядковый индекс структуры в массиве структур, от 0 до SENSOR_MAX
 * - адрес датчика на шине от SENSOR_ADR_MASK до 0 с шагом 2. Обратите внимание что адрес сдвинут на 1 бит влево и младший бит всегда 0
 */ 


#ifndef SENSORS_H_
#define SENSORS_H_

#include "Clock.h"

// Адреса сенсоров: 
//	Отображаемый адрес датчика определяется битами 1,2,3 сдвинутыми на один бит вправо. 
//	Например для SENSOR_DS18B20 это будет 6 => 0b00001100 маска битов 1,2,3(d0b00001110) => сдвиг на один бит влево => 0b00000110=6
//	Дополнительные адреса возвращают датчики на радиоканале. Они полностью определяются прошивкой МК на стороне радиодатчика.
#define SENSOR_DS18B20		0b00001100				// на шине 1-ware датчик пока может быть только один, с адресом 6.
#define SENSOR_BMP180		0b00000110				//Адрес BMP180 для отображения, хранится в структуре sSensor, не путать с BMP180_ADDRESS!
#define BMP180_ADDRESS		0b11101110				//Адрес BMP180 на шине I2C, зашит в самом датчике
#define SENSOR_LM75AD		0b10011110				//Адрес LM75AD

#define SENSOR_MAX			3						//Количество поддерживаемых сенсоров, не более 8. Это ограничение алгоритма, но на самом деле еще и длина выводимой строки и количество памяти влияет
#if (SENSOR_MAX>8)
	#error "Value of the SENSOR_MAX must be less than 8"	//Сообщить если количество датчиков выходит за ограничение разрядности адреса
#endif

#define SENSOR_LEN_NAME		3						//Длина имени датчика
#define SENSOR_MAX_PERIOD	30						//Период в минутах в течении которого не поступала информация от датчика. По истечении этого периода, информация считается недействительной
#define SENSOR_TEST_REPEAT	500						//Период повторения измерения для режима тестирования

struct sSensor{										//Датчик температуры, давления и т.п. В eeprom сохраняются только Adr, State, Name
	u08 Adr;										//Адрес датчика. Значимыми являются биты 1,2,3. Бит 0 всегда равен 0 и фактический адрес сдвинут на 1 влево. Это сделано для того что бы различать сигнал от датчика и от ИК-приемника
	u08 Value;										//Значение измеряемой величины, для датчика давления смещение от PressureBase в мм.рт.ст
	u08 State;										//Состояние датчика. Тестирование, батарея разряжена и пр. Причем старшие 4 бита определяются внешним датчиком, а младшие 4 вычисляются внутри программы часов
	u08 SleepPeriod;								//Счетчик обратного отсчета периода в течении которого не поступал сигнал от датчика. В минутах
	char Name[SENSOR_LEN_NAME];						//Имя датчика для вывода на дисплей
};

#define PressureBase	641							//Смещение для хранения величины давления, что бы уменьшить разрядность с 32 бит до 16
#define PressureNormal(shortPress)	((u16)(PressureBase+shortPress))
#define PressureShort(pressure)	((u08)(pressure-PressureBase))

//------------------------------- Биты байта State в структуре sSensor
// TODO: Объединить описание структуры байта статуса в один совместный файл с прошивкой датчика
													//Биты от 7 до 4 поступают от датчика
#define	SENSOR_NO_SENSOR	7						//Датчик не обнаружен 1 - нет датчика на шине (у радиодатчика это означает, что ответ от радиодатчика есть, но внутри него сенсор не ответил)
#define	SENSOR_LOW_POWER	6						//Низкое напряжение батарейки 1 - слишком низкое напряжение на батарее
#define	SENSOR_NO_RF		5						//Имеет смысл только для радиодатчика. 1 - радиодатчик не прислал вовремя пакет ответа
#define	SENSOR_PRESSURE		4						//Тип датчика: 1-датчик давления
													//Биты от 3 до 0 обрабатываются самими часами
#define SENSOR_SHOW_ON		3						//Разрешен вывод значения датчика на дисплей
#define SENSOR_RESERV_CLK1	2						//-------- Зарезервировано для будущего использования
#define SENSOR_RESERV_CLK2	1
#define SENSOR_RESERV_CLK3	0

#define SENSOR_ADR_MASK		0b00001110				//Маска адреса датчика
#define SENSOR_STATE_MASK	((1<<SENSOR_NO_SENSOR) | (1<<SENSOR_LOW_POWER) | (1<<SENSOR_NO_RF) | (1<<SENSOR_PRESSURE))	//Маска флагов статуса датчиков (не внутренние флаги!)
#define SensorSetInBus(st)	ClearBit(st.State, SENSOR_NO_SENSOR)		//Датчик на шине есть. Для радиодатчика это означает что есть ответ от радиопередатчика, но может нет ответа от самого датчика. Наличи датчика в радиодатчике проверяется через значение Value (см. флаги TMPR_NO_SENS...)
#define SensorNoInBus(st)	SetBit(st.State, SENSOR_NO_SENSOR)			//Пометить отсутствие датчика на шине
#define SensorFlgClr(st)	(st.State & (~SENSOR_STATE_MASK))			//Сброс флагов самого датчика
#define SensorIsNo(st)		BitIsSet(st.State, SENSOR_NO_SENSOR)		//Датчик отсутствует на шине?
#define SensorIsSet(st)		BitIsClear(st.State, SENSOR_NO_SENSOR)		//Датчик отвечает на шине?
#define	SensorBatareyIsLow(st)	BitIsSet(st.State, SENSOR_LOW_POWER)	//Батарея разряжена
#define SensotTypeTemp(st)	ClearBit(st.State, SENSOR_PRESSURE)			//Это датчик температуры
#define SensorTypePress(st)	SetBit(st.State, SENSOR_PRESSURE)			//Это датчик давления
#define SensorIsPress(st)	BitIsSet(st.State, SENSOR_PRESSURE)			//Это датчик давления?
#define SensorRFInBus(st)	ClearBit(st.State, SENSOR_NO_RF)			//Радиодатчик вовремя не ответил
#define SensorRFNoBus(st)	SetBit(st.State, SENSOR_NO_RF)
#define SensorRFIsNo(st)	BitIsSet(st.State, SENSOR_NO_RF)

#define SensorShowOn(st)		SetBit(st.State, SENSOR_SHOW_ON)		//Включить отображение датчика на дисплей
#define SensorShowOff(st)		ClearBit(st.State, SENSOR_SHOW_ON)		//Выключить
#define SensorIsShow(st)		BitIsSet(st.State, SENSOR_SHOW_ON)		//Вывод на дисплей для датчика включен

//Команды для функции отображения значения датчиков. Значения выбраны исходя из того что отрицательные температуры выводятся в дополнительном коде от 0xc90 (-55 C) до 0xff (-1 C)
#define TMPR_NO_SENS		0x80					//Нет датчика

//Режимы отображения датчика
#define SENSOR_SHOW_NORMAL	0						//Отображение по мере необходимости (в бегущей строке например)
#define SENSOR_SHOW_TEST	1						//Режим немедленного отображения результата измерения

void SensorIni(void);								//Инициализация описания датчиков
u08 SetSensor(u08 Adr, u08 Stat, u08 Value);		//Запись адреса-статуса и значения сенсора, определение текущего режима отображения значения датчика
struct sSensor *SensorNum(u08 id);					//Возвращает указатель на сенсор id

#define SensorFromIdx(id)	(*SensorNum(id))

#endif /* SENSORS_H_ */