/*
 * Работа с модулем nRF24L01P через hard-spi
 * Прием пакета и передача ответа.
 * Принимает байт температуры. Если нет датчика температуры то принимается SENSOR_NO = (0xc0)
 * Поскольку ответный пакет можно не успеть загрузить в передатчик, то используется двухтактная передача.
 * В первом такте ответ передается для минимальной длительности сна, а во втором действительная.
 * Таким образом часы успевают подготовить ответ для необходимого радиодатчика.
 * В ответ передается длительность периода засыпания. Длительность периода передается в виде трех
 * байт. Первый байт это значение прескаллера для таймера WDT
 * Второй и третий байты это счетчик срабатываний таймера WDT. Младший байт счетчика передается вперед.
 * Пример кода ответа:
 * 		BufTx[0] = ATTINY_13A_8S_SLEEP;		//Команда для таймера сна, заснуть на 8 секунд
 *		BufTx[1] = 0x00;					//Старший байт счетчика сна
 *		BufTx[2] = 0x01;					//Младший байт счетчика
 *		nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | 0b00000000, BUF_LEN, BufTx);	//Записать ответ для канала 0
 */ 


#ifndef NRF24L01P_H_
#define NRF24L01P_H_

#define nRF_PIPE				5				//Номер используемого канала
#define nRF_SEND_LEN			4				//Длина передаваемого пакета данных. Должна совпадать со значением nRF_SEND_LEN в файле конфигурации часов ....\ClockMatrix\nRF24L01P.h
#define nRF_TMPR_ATTNY13_SENSOR	0x01			//Датчик температуры на Attiny13. Должна совпадать со значением nRF_TMPR_ATTNY13_SENSOR в файле конфигурации часов ....\ClockMatrix\nRF24L01P.h
#define nRF_ACK_LEN				3				//Длина ответа от часов при приеме пакета от радиодатчика. Должна совпадать со значением nRF_ACK_LEN в файле конфигурации часов ....\ClockMatrix\nRF24L01P.h
#define nRF_RESERVED_BYTE		0				//Байт зарезервированный для будущего использования
#define nRF_SENSOR_NO			0xfa00			//Сенсора на шине 1-Ware нет.	Должна совпадать со значением SENSOR_NO в файле конфигурации часов ...\TempSensorDSandNRF24\nRF24L01P.h


#if ((nRF_ACK_LEN<1) && (nRF_ACK_LEN>32))
	#error "Incorrect value nRF_ACK_LEN. The nRF_ACK_LEN should be between 1 to 32"
#endif

#include <avr/io.h>
#include "Clock.h"
#include "avrlibtypes.h"
#include "nRF24L01P Reg.h"
#include "bits_macros.h"

#define nRF_DDR					DDRC			//SPI к nRF24L01+
#define nRF_PORTIN				PINC
#define nRF_PORT				PORTC
#define nRF_MOSI				PINC2
#define nRF_MISO				PINC5
#define nRF_SCK					PINC4
#define nRF_CSN					PINC3			//Выбор кристалла
//#define nRF_CE					PINC2			//Запуск операции. Если вывод CE постоянно соединен с питанием то закоментировать
//#define nRF_IRQ					PINC4			//Прерывание от модуля

#define nRF_SELECT()			ClearBit(nRF_PORT, nRF_CSN)
#define nRF_DESELECT()			SetBit(nRF_PORT, nRF_CSN)

#ifdef nRF_CE
#define nRF_GO()				SetBit(nRF_PORT, nRF_CE)	//Пуск приемопередтчика
#define nRF_STOP()				ClearBit(nRF_PORT, nRF_CE)	//Стоп
#else
#define nRF_GO()				//Если аппаратное управление включением приемника не используется то команды не нужны
#define nRF_STOP()				
#endif

#ifdef nRF_IRQ
#define nRF_IS_RECIV()			BitIsClear(nRF_PORTIN, nRF_IRQ)	//Какая-то операция завершена
#define nRF_NO_RECIV()			BitIsSet(nRF_PORTIN, nRF_IRQ)	//Ничего нет
#endif

//Настройка прескаллера WDT для ATTINY13A
#define ATTINY_13A_WDP0 0
#define ATTINY_13A_WDP1 1
#define ATTINY_13A_WDP2 2
#define ATTINY_13A_WDP3 5
#define ATTINY_13A_16MS_SLEEP	((0<<ATTINY_13A_WDP3) | (0<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	//срабатывание собаки через 16 мс
#define ATTINY_13A_05S_SLEEP	((0<<ATTINY_13A_WDP3) | (1<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	// -/- 0,5 секунд
#define ATTINY_13A_1S_SLEEP		((0<<ATTINY_13A_WDP3) | (1<<ATTINY_13A_WDP2) | (1<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	// -/- 1 секунду
#define ATTINY_13A_8S_SLEEP		((1<<ATTINY_13A_WDP3) | (0<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (1<<ATTINY_13A_WDP0))	// -/- 8 секунд

#define nRF_WDT_INTERVAL_S		8				//Интервал просыпания WDT
#define nRF_MEASURE_NORM_MIN	10				//Период измерения внормальном режиме, в минутах. Режим WDT должен быть ATTINY_13A_8S_SLEEP
#define nRF_MEASURE_TEST_SEC	1				//Период измерения в тестовом режиме в секундах. Режим WDT должен быть ATTINY_13A_1S_SLEEP
#define nRF_INTERVAL_NORM		((u16)(nRF_MEASURE_NORM_MIN*60)/nRF_WDT_INTERVAL_S)
#define nRF_INTERVAL_NORM_LSB	((u08)nRF_INTERVAL_NORM & 0xff)			//Младший байт
#define nRF_INTERVAL_NORM_MSB	((u08)(nRF_INTERVAL_NORM>>8) & 0xff)	//Старший байт

#define nRF_PERIOD_TEST			100				//Период проверки состояния модуля приемо-передатчика

void nRF_Init(void);
void nRF_StartRecive(void);						//Включить модуль на прием

#endif /* NRF24L01P_H_ */