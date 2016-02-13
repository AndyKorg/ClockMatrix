/*
 * OneWare.c
 * Обслуживание на шине 1-Ware с помощью прерываний
 * Created: 26.07.2014 15:24:18
 * ver. 1.2
 */ 


#ifndef ONEWARE_H_
#define ONEWARE_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "avrlibtypes.h"
#include "bits_macros.h"

typedef void (*ONEWARE_EVENT)(void);						//тип указателя на функцию окончания обработки команды на шине 1-Ware

#define ONE_WARE_BUF_LEN	2
extern volatile u08	owStatus,								//Текущий статус шины 1-Ware, флаговая переменная
					owRecivBuf[ONE_WARE_BUF_LEN],			//Буфер для принятых байт
					owSendBuf[ONE_WARE_BUF_LEN];			//Буфер для передаваемых байт
//Флаги переменной OneWareStatus
#define OW_BUSY			7									//Бит занятости шины 1-ware, 0 - свободно, 1 занято
#define owBusy()		SetBit(owStatus, OW_BUSY)			//Занять шину
#define owFree()		ClearBit(owStatus, OW_BUSY)			//Освободить шину
#define owIsFree()		BitIsClear(owStatus, OW_BUSY)		//Шина свободна
#define owIsBusy()		BitIsSet(owStatus, OW_BUSY)			//Шина занята
#define OW_DEVICE		6									//Присутствие устройства на шине
#define owDeviceSet()	SetBit(owStatus, OW_DEVICE)			//Установить флаг присутствия устройства на шине
#define owDeviceNo()	ClearBit(owStatus, OW_DEVICE)		//нет устройства
#define owDeviceIsSet()	BitIsSet(owStatus, OW_DEVICE)		//Проверить присутствие устройства на шине
#define owDeviceIsNo()	BitIsClear(owStatus, OW_DEVICE)		//Проверить отсутствие устройства на шине
#define OW_ONLY_SEND	5									//Только передача без приема
#define owSendOnly()	SetBit(owStatus, OW_ONLY_SEND)		//Только передача команды
#define owSendAndReciv() ClearBit(owStatus, OW_ONLY_SEND)	//Передать и принять
#define owIsSendOnly()	BitIsSet(owStatus, OW_ONLY_SEND)	//Режим передачи
#define owIsSndAndRcv()	BitIsClear(owStatus, OW_ONLY_SEND)	//Режим и приема и передачи
#define OW_RECIV_STAT	4									//1 - Текущий режим автомата - прием байта, 0 - передача
#define owRecivStat()	SetBit(owStatus, OW_RECIV_STAT)		//Автомат переведен в режим приема байта
#define owSendStat()	ClearBit(owStatus, OW_RECIV_STAT)	//Режим передачи байта
#define owIsRecivStat()	BitIsSet(owStatus, OW_RECIV_STAT)	//Идет прием
#define owIsSendStat()	BitIsClear(owStatus, OW_RECIV_STAT)	//Идет передача

void owInit(void);
void owExchage(void);										//Начать обмен по шине 1-Ware
volatile ONEWARE_EVENT owCompleted;							//Функция вызываемая по окончании обмена.
volatile ONEWARE_EVENT owError;								//Функция вызываемая при возникновении ошибки на шине

//------------- Команды шины 1-Ware
#define ONE_WARE_SEARCH_ROM		0xf0						//Поиск адресов - используется при универсальном алгоритме определения количества и адресов подключенных устройств
#define ONE_WARE_READ_ROM		0x33						//Чтение адреса устройства - используется для определения адреса единственного устройства на шине
#define ONE_WARE_MATCH_ROM		0x55						//Выбор адреса - используется для обращения к конкретному адресу устройства из многих подключенных
#define ONE_WARE_SKIP_ROM		0xcc						//Игнорировать адрес - используется для обращения к единственному устройству на шине, при этом адрес устройства игнорируется (можно обращаться к неизвестному устройству)

#endif /* ONEWARE_H_ */