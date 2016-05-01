/*
 * Протокол общения с esp8266
 * см. ..\Web_base\app\include\customer_uart.h
 */ 


#ifndef ESP8266HAL_H_
#define ESP8266HAL_H_

#define ICACHE_FLASH_ATTR
#include "C:\Espressif\Web_base\app\user\Include\clock_web.h"

#include "avrlibtypes.h"
#include "Volume.h"

#define ESP_WHITE_START	10000									//Ожидание перед тестированием модуля

// Команды для часов передаваемые через uart
#define CLK_CMD_TYPE	0b11000000								//Биты типа команды
#define CLK_CMD_WRITE	0b10000000								//Команда записи
#define CLK_CMD_TEST	0b11000000								//Команда тестирования
#define ClkWrite(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_WRITE)	//Запись в часы или модуль
#define ClkTest(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_TEST)	//Команда тестирования
#define ClkRead(cmd)	(cmd & (~CLK_CMD_WRITE))				//Прочитать из часов или модуля
#define ClkIsRead(cmd)	((cmd & CLK_CMD_TYPE) == 0)				//Команда чтения
#define ClkIsWrite(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_WRITE)	//Команда записи
#define ClkIsTest(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_TEST)	//Команда тестирования
#define ClkCmdCode(cmd)	(cmd & (~CLK_CMD_TYPE))					//Код команды без типа

#define CLK_NODATA		0										//Индикатор отсутствия данных в команде. Введено для удобства

extern u08* espStationIP;										//Адрес IP в виде строки XXX.XXX.XXX.XXX

void StartGetTimeInternet(void);								//Запускает процесс запроса времени из Интернета
u08 espInstalled(void);											//Возвращает 1 если модуль установлен
void espInit(void);												//Проверка наличия модуля esp
void espUartTx(u08 Cmd, u08* Value, u08 Len);					//Отправка команды на модуль
void espWatchTx(void);											//Передать в модуль текущее время
void espVolumeTx(enum tVolumeType Type);						//Передать в модуль режим работы сигналов и громкости
void espSendSensor(u08 numSensor);								//Передать в модуль значение датчика NumSensor
void espGetIPStation(void);										//Запуск получения IP адреса модуля в режиме station
void espNetNameSet(void);										//Авторизация в сети по имени и парою с sd карты

#endif /* ESP8266HAL_H_ */