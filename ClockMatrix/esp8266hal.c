/*
 * Протокол общения с esp8266
 * см. ..\Web_base\app\include\customer_uart.h
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include "FIFO.h"
#include "Clock.h"
#include "esp8266hal.h"
#include "bits_macros.h"
#include "EERTOS.h"
#include "Alarm.h"
#include "Sound.h"
#include "sensors.h"
#include "i2c.h"
#include "Display.h"
#include "eeprom.h"

#ifdef DEBUG
	#include "Display.h"
#endif

#ifdef ATMEGA644
	#ifdef DEBUG
		#include "usartDebug.h"
	#endif
#endif

#ifdef ATMEGA644
	#define ESP_UART_DATA	UDR1								//Регистр данных для esp
	#define ESP_UCSRA		UCSR1A
	#define ESP_UCSRB		UCSR1B
	#define ESP_TXC			TXC1								//Буфер передачи пуст
	#define ESP_RXC			RXC1								//Принят байт
	#define ESP_UDRE		UDRE1
	#define ESP_UDRIE		UDRIE1								//Разрешить прерывания по окончании передачи
	#define ESP_RXCIE		RXCIE1								//Разрешить прерывания по окончания приема байта
	#define ESP_UARTRX_vect	USART1_RX_vect						//Прерывание по приему байта от esp
	#define ESP_UARTTX_vect USART1_UDRE_vect					//Прерывание по окончании передачи байта в esp
#else
	#define ESP_UART_DATA	UDR									//Регистр данных для esp
	#define ESP_UCSRA		UCSRA
	#define ESP_UCSRB		UCSRB
	#define ESP_TXC			TXC									//Буфер передачи пуст
	#define ESP_RXC			RXC									//Принят байт
	#define ESP_UDRE		UDRE
	#define ESP_UDRIE		UDRIE								//Разрешить прерывания по окончании передачи
	#define ESP_RXCIE		RXCIE								//Разрешить прерывания по окончания приема байта
	#define ESP_UARTRX_vect	USART_RXC_vect						//Прерывание по приему байта от esp
	#define ESP_UARTTX_vect USART_UDRE_vect						//Прерывание по окончании передачи байта в esp
#endif

#define espSendRdy()	BitIsSet(ESP_UCSRA, ESP_TXC)			//Передатчик в esp готов принять данные
#define espSendWhite()	while(BitIsClear(ESP_UCSRA, ESP_TXC))	//Ожидание окончания передачи в  esp
#define espRxWhile()	while(BitIsClear(ESP_UCSRA, ESP_RXC))	//Ожидание приема байта от esp
#define espRxStart()	SetBit(ESP_UCSRB, ESP_RXCIE)			//Разрешить прерывание при приеме байта
#define espRxStop()		ClearBit(ESP_UCSRB, ESP_RXCIE)
#define epsTxStart()	SetBit(ESP_UCSRB, ESP_UDRIE)			//Разрешить прерывание по готовности передатчика
#define espTxStop()		ClearBit(ESP_UCSRB, ESP_UDRIE)			//запретить

#define ESP_RESET_PORT	PORTA									//Порт ноги сброса esp
#define ESP_RESET_DDR	DDRA
#define ESP_RESET_BIT	PORTA3
#define espResetInit()	do{SetBit(ESP_RESET_DDR, ESP_RESET_BIT); SetBit(ESP_RESET_PORT, ESP_RESET_BIT);}while(0)
#define espReset()		ClearBit(ESP_RESET_PORT, ESP_RESET_BIT)
#define espStart()		SetBit(ESP_RESET_PORT, ESP_RESET_BIT)

#define ESP_ERR_TIMEOUT	2000										//Максимальное время ожидания ответа от модуля, мс

#define CMD_CLCK_PILOT1		0xaa								//Первый байт пилота команды
#define CMD_CLCK_PILOT2		0x55								//Второй байт пилота команды
#define CLK_CMD_PILOT_NUM	4									//Длина пилота для команды

#define CLK_CMD_EMPTY	0x00									//Специальная пустая команда, или буфер команды пуст
#define CLK_TIMEOUT		3000									//Таймаут ожидания завершения ответа от часов мс

#define CLK_MAX_LEN_CMD	32										//Максимальная длина команды uart. Должна быть степенью двойки см. fifo.h

//Буфер FIFO данных для приема и передачи
FIFO(CLK_MAX_LEN_CMD)
	espTxBuf, espRxBuf;

//############################ Команды и переменные UART  ###########################################################################################
typedef void (*VOID_CLK_UART_CMD)(u08 cmd, u08* pval, u08 valLen);//Сама команда и её данные

typedef const PROGMEM struct{									//Команда UART
	u08 CmdCode;
	VOID_CLK_UART_CMD Func;										//Если Value не NULL, то функция не вызывается
} PROGMEM pEspUartCmd;

//Команды поступающие от модуля esp
void UartWatchSet(u08 cmd, u08* pval, u08 valLen);				//Записать время от esp
void UartAlarmSet(u08 cmd, u08* pval, u08 valLen);				//Сохранить значение будильников
void UartVolumeSet(u08 cmd, u08* pval, u08 valLen);				//Громкость
void UartSensorSet(u08 cmd, u08* pval, u08 valLen);				//Управление датчиками
void UartFontSet(u08 cmd, u08* pval, u08 valLen);				//Установить шрифт
void UartGetAll(u08 cmd, u08* pval, u08 valLen);				//Прочитать все установки часов
void UartBaseModeSet(u08 cmd, u08* pval, u08 valLen);			//Возврат в основной режим при получении команды CLC_STOP
void UartStationIP(u08 cmd, u08* pval, u08 valLen);				//Получен IP адрес station
void UartHorizontalSpeed(u08 cmd, u08* pval, u08 valLen);		//Установить скорость бегущей строки

//-------------------- Список команд и переменных для приема по UART ---------------------------------------------------
#define UART_CMD_NUM_MAX 9										//Количество команд uart
const pEspUartCmd PROGMEM espUartCmd[UART_CMD_NUM_MAX] = {
	/*	CmdCode, 		Func */
	{CLK_WATCH,			UartWatchSet},
	{CLK_ALARM, 		UartAlarmSet},
	{CLK_VOLUME,		UartVolumeSet},
	{CLK_SENS,			UartSensorSet},
	{CLK_FONT,			UartFontSet},
	{CLK_STOP,			UartBaseModeSet},
	{CLK_ALL,			UartGetAll},
	{CLK_ST_WIFI_IP,	UartStationIP},
	{CLK_HZ_SPEED, 		UartHorizontalSpeed},
		
/*
--#define CLK_NTP_START	0x03	//Запустить получение времени от ntp сервера
#define CLK_STOP		0x08	//Остановить операцию (тестирование или ожидание)

//Ответы
#define CLK_NTP_ERROR	0x70	//Ошибка обращения к серверу ntp
*/	
};
#define espCmdCode(id)			(pgm_read_byte(&espUartCmd[id].CmdCode))
#define espCmdExec(id)			*((VOID_CLK_UART_CMD*)pgm_read_word(&espUartCmd[id].Func))

volatile u08  espStatusFlags;									//Состояние модуля - флаг таймаут, счетчик попыток связи с модулем
//								7								
//								6
//								5	свободные биты
//								4
#define ESP_ATTEMPT_MASK		0b00001111						//Маска счетчика попыток связи с модулем

#define espAttemptLinkSet()		do{espStatusFlags |= ESP_ATTEMPT_MASK;}while(0)
#define espAttemptIsEnded()		((espStatusFlags & ESP_ATTEMPT_MASK)==0)	//Попытки исчерпаны
#define espTimeoutGoing()		(espStatusFlags & ESP_ATTEMPT_MASK)	//Работает таймер таймаута

void espTimeoutError(void);										//Проверка времени ответа модуля

u08* espStationIP = NULL;										//Адрес IP в виде строки XXX.XXX.XXX.XXX

/************************************************************************/
/* Передать данные в esp                                                */
/************************************************************************/
void espUartTx(u08 Cmd, u08* Value, u08 Len){
	if espModuleIsNot()											//Нет модуля, ничего не делаем
		return;
	while(FIFO_SPACE(espTxBuf) < (Len+6)){						//Ждем пока буфер не освободится
		TaskManager();
		if espModuleIsNot(){									//Не удалось определить наличие модуля
			return;
		}
	}

	u08 FlagInt = SREG & SREG_I;
	cli();														//Формируется пакет
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT1);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT2);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT1);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT2);
	FIFO_PUSH(espTxBuf, Len);
	while(Len){
		FIFO_PUSH(espTxBuf, *Value++);
		Len--;
	}
	FIFO_PUSH(espTxBuf, Cmd);
	epsTxStart();
	if (FlagInt)
		sei();
	espAttemptLinkSet();
	SetTimerTask(espTimeoutError, ESP_ERR_TIMEOUT);				//Таймаут ответа от модуля
}

#ifdef DEBUG
void UartDebugOut(u08 *ptr, u08 Len){
	if (Len < CLK_MAX_LEN_CMD)
		espUartTx(ClkWrite(CLK_DEBUG), ptr, Len);
	else{
		u08 err[] = "buf owerflow";
		espUartTx(ClkWrite(CLK_DEBUG), err, sizeof(err));
	}
}
#endif

/************************************************************************/
/* Начальная установка переменных модуля                                */
/************************************************************************/
void espVarInit(void){
	u08 i;
	
	espNetNameSet();											//Авторизовать в сети wifi как station если есть имя сети и пароль на sd-карте
	espWatchTx();												//Время
	for (i=0;i<ALARM_MAX;i++){									//Будильники
		espUartTx(ClkWrite(CLK_ALARM), (u08*) ElementAlarm(i), sizeof(struct sAlarm));
	}
	espVolumeTx(vtAlarm);										//Громкости и сигналы
	espVolumeTx(vtButton);
	espVolumeTx(vtEachHour);
	for(i=0;i<SENSOR_MAX;i++)									//Датчики
		espSendSensor(i);
	i = FontIdGet();
	espUartTx(ClkWrite(CLK_FONT), &i, 1);						//Шрифт
	if (RTC_IsNotValid())										//Был сбой питания, запустить запрос времени из Интернета
		StartGetTimeInternet();

	u08 HZConst[HORZ_SCROLL_CMD_COUNT];							//Скорость бегущей строки - нужно 2 байта, один для передачи количества шагов, один для текущего значения скорости				
	HZConst[0] = HORZ_SCROLL_STEPS;
	HZConst[1] = HorizontalSpeed/HORZ_SCROOL_STEP;
	espUartTx(ClkWrite(CLK_HZ_SPEED), HZConst, HORZ_SCROLL_CMD_COUNT);
}

/************************************************************************/
/* Начать тест модуля													*/
/************************************************************************/
void espCheckStart(void){
	u08 i;
	espUartTx(ClkTest(CLK_CMD_EMPTY), &i, 1);					//Отправляем тестовую команду
}

/************************************************************************/
/* Таймаут ответа от модуля												*/
/************************************************************************/
void espTimeoutError(void){
	if (espAttemptIsEnded()){								//Остались попытки?
		espModuleRemove();									//Считаем, что модуль отсутствует
	}
	else{
		espStatusFlags--;									//Минус одна попытка
		espReset();											//Сбрасываем
		FIFO_FLUSH(espTxBuf);
		//SetTimerTask(espCheckStart, CLK_TIMEOUT);			//И запускаем проверку модуля
		espStart();
		SetTimerTask(espTimeoutError, ESP_WHITE_START);		//Ждем пока модуль не перестартует
	}
}

/************************************************************************/
/* Поиск и исполнение команды в принятых данных							*/
/************************************************************************/
void espParse(void){
	u08 Len = FIFO_FRONT(espRxBuf);
	u08* ptrData = malloc(Len);
	
	if (ptrData == NULL)										//Не хватило памяти для обработки команды, подождем может появится
		SetTask(espParse);
	else{
		u08* Data = ptrData;
		FIFO_POP(espRxBuf);
		for(u08 i=Len;i;i--){									//Извлекаем данные из буфера
			*Data = FIFO_FRONT(espRxBuf);
			Data++;
			FIFO_POP(espRxBuf);
		}
		u08 Cmd = FIFO_FRONT(espRxBuf);
		FIFO_POP(espRxBuf);
		for (u08 i=0; i < UART_CMD_NUM_MAX; i++) {
			if (espCmdCode(i) == ClkCmdCode(Cmd)){				//Команда найдена
				((VOID_CLK_UART_CMD)&espCmdExec(i))(Cmd, ptrData, Len);//Отрабатываем
				break;
			}
		}
		free(ptrData);
	}
}
/************************************************************************/
/* Принят байт от esp                                                   */
/************************************************************************/
ISR(ESP_UARTRX_vect){
	u08 rxbyte = ESP_UART_DATA;
	static u08 CountByte = 0, CountData = 0;
	
	#define CountByteVal()		(CountByte & 0x3f)
	#define SetCustomTxtCmd()	SetBit(CountByte, 7)			//Специальная команда для произвольного текста
	#define CustomTxtCmdIsSet()	BitIsSet(CountByte, 7)
	#define CustomTxtEnd()		SetBit(CountByte, 6)			//Был конец произвольной строки
	#define CustomTxtIsNoEnd()	BitIsClear(CountByte, 6)			
	
	if (CountByteVal() < CLK_CMD_PILOT_NUM){					//Ожидаются пилот-байты
		if (rxbyte == ((CountByte & 1)?CMD_CLCK_PILOT2:CMD_CLCK_PILOT1)) //Ждем соответствующий байт пилота
			CountByte++;
		else
			CountByte = 0;
	}
	else{														//Есть синхронизация
		espModuleSet();											//Флаг наличия модуля установить
		espAttemptLinkSet();									//Максимальное количество попыток связи с модулем
		if (CountByteVal() == CLK_CMD_PILOT_NUM){				//Принята длина данных
			CountData = rxbyte;
			CountByte++;
			if (rxbyte > CLK_MAX_LEN_CMD){						//Если количество данных больше максимальной длины команды, то это специальная команда CLK_CUSTOM_TXT
				SetCustomTxtCmd();
				CountData++;									//Добавляю единицу что бы код команды то же принять как данные
				ClearDisplay();									//Подготовить дисплей
				CreepOn(1);
			}
			else{
				if ((rxbyte+2)>FIFO_SPACE(espRxBuf)){			//При нехватке места в буфере обычная команда сбрасывается
					CountByte = 0;								//Число 2 это байт длины и байт команды
				}
				else{											//есть свободное место в буфере, суем туда длину данных
					FIFO_PUSH(espRxBuf, rxbyte);
				}
			}
		}
		else{
			if CustomTxtCmdIsSet(){								//Специальная команда произвольного текста, пишется сразу в буфер дисплея
				if ((CountData != 1) && CustomTxtIsNoEnd()){	//последний байт это код команды, его надо игнорировать и конца строки еще не было
					if (rxbyte){								//Нулевой байт это конец строки, после него ничего не выводится
						if (rxbyte == '_')						//Пока подчеркивание переделывается в пробел из-за бага в прошивке esp8266
							sputc(S_SPACE, UNDEF_POS);
						else
							sputc(rxbyte, UNDEF_POS);
						sputc(S_SPICA, UNDEF_POS);
					}
					else{
						sputc(S_SPICA, UNDEF_POS);
						CustomTxtEnd();
					}
				}
			}
			else{
				FIFO_PUSH(espRxBuf, rxbyte);					//Принимается поток данных и код команды
			}
			if (CountData){
				CountData--;
			}
			else{												//Все принято
				if (CustomTxtCmdIsSet())						//Если специальная команда произвольного текста, то запустить вывод температуры и прочего
					ShowDate();
				else
					SetTask(espParse);
				CountByte = 0;									//Запустить цикл по новой
			}
		}
	}
}

/************************************************************************/
/* Передатчик пуст, можно дальше передавать                             */
/************************************************************************/
ISR(ESP_UARTTX_vect){
	if( FIFO_IS_EMPTY(espTxBuf) ) {								//если данных в fifo больше нет то запрещаем это прерывание
		espTxStop();
	}
	else {														//иначе передаем следующий байт
		u08 txbyte = FIFO_FRONT(espTxBuf);
		FIFO_POP(espTxBuf);
		ESP_UART_DATA = txbyte;
	}
}

/************************************************************************/
/* Инициализация USART. Если IntOn - 1 то включаются прерывания         */
/************************************************************************/
#define ESP_SERIAL_INT_ON	1
#define ESP_SERIAL_INT_OFF	0
inline void espSerialIni(u08 IntOn){

#ifdef ATMEGA644
	//-------- Настройка порта для модуля
	#undef BAUD											//Для пересчета скорости для модуля
//	#define BAUD 9600
	#define BAUD 115200
	#include <util/setbaud.h>
	UCSR1A = 0x00;										//Все по умолчанию
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) |					//Разрешить прием и передачу
				((IntOn?1:0)<<RXCIE1);					//прерывание от приемника включить при необходимости
	UCSR1C = (0<<UMSEL11) | (0<<UMSEL10) |				//Asynchronius USART
			(0<<UPM11) | (0<<UPM10) |					//Parity off
			(0<<USBS1) |								//Stop bit = 1
			(0<<UCSZ12) | (1<<UCSZ11) | (1<<UCSZ10) |	//8 бит данных
			(0<<UCPOL1);								//Polary XCK не важно т.к. не используется в синхронном режиме
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	#if USE_2X											//Если скорости не хватит подключается умножитель на 2
		UCSR1A |= (1 << U2X1);
	#else
		UCSR1A &= ~(1 << U2X1);
	#endif
#else
	//-------- Настройка порта для модуля
	#undef BAUD											//Для пересчета скорости для модуля
//	#define BAUD 115200
	#define BAUD 19200
	#include <util/setbaud.h>
	ESP_UCSRA = 0x00;									//Все по умолчанию
	ESP_UCSRB = (1<<RXEN) | (1<<TXEN) |					//Разрешить прием и передачу
				((IntOn?1:0)<<RXCIE);					//прерывание от приемника включить при необходимости
	UCSRC =  (1<<URSEL) |								//Запись в UCSRC
			 (0<<UMSEL) |								//Asynchronius USART
			 (0<<UPM1) | (0<<UPM0) |					//Parity off
			 (0<<USBS) |								//Stop bit = 1
			 (0<<UCSZ2) | (1<<UCSZ1) | (1<<UCSZ0) |		//8 бит данных
			 (0<<UCPOL);								//Polary XCK не важно т.к. не используется в синхронном режиме
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X											//Если скорости не хватит подключается умножитель на 2
		UCSRA |= (1 << U2X);
	#else
		UCSRA &= ~(1 << U2X);
	#endif
#endif
FIFO_FLUSH(espTxBuf);
FIFO_FLUSH(espRxBuf);
}

/************************************************************************/
/* Инициализация интерфейса и запуск таймаута проверки модуля			*/
/************************************************************************/
void espInit(void){
	espResetInit();										//Ногу ресет модуля инициализировать
	espReset();											//Сбросить модуль
	espSerialIni(ESP_SERIAL_INT_ON);					//Инициализировать USART
	espAttemptLinkSet();								//Куча попыток
	espStart();											//Модуль запустить - т.е. ногу ресет модуля в высокое состояние
	espModuleRemove();									//Считаем что модуль отсутствует
	SetTimerTask(espTimeoutError, ESP_WHITE_START);		//Куча времени на запуск модуля
}

/************************************************************************/
/* Возвращает 1 если модуль установлен                                  */
/* Выведено в функцию, т.к. используется в меню                         */
/************************************************************************/
u08 espInstalled(void){
	return espModuleIsSet()?1:0;
}

/************************  КОНКРЕТНЫЕ КОМАНДЫ ***************************/

/************************************************************************/
/* Установить скорость бегущей строки			                        */
/************************************************************************/
void UartHorizontalSpeed(u08 cmd, u08* pval, u08 valLen){
	if ClkIsWrite(cmd){
		if (valLen == HORZ_SCROLL_CMD_COUNT){			//Должно быть два байта
			if ((pval[0] == HORZ_SCROLL_STEPS) && (pval[1]<=HORZ_SCROLL_STEPS)){	//В допустимом диапазоне
				HorizontalSpeed = HORZ_SCROOL_STEP*((u16)pval[1]);
				EeprmStartWrite();						//Записать в eeprom новое значение
			}
		}
	}
}

/************************************************************************/
/* Запускает процесс запроса времени из Интернета                       */
/************************************************************************/
void StartGetTimeInternet(void){
	u08 i;
	espUartTx(ClkWrite(CLK_NTP_START), &i, 1);
}		

/************************************************************************/
/* принять из модуля значение будильника								*/
/************************************************************************/
void UartAlarmSet(u08 cmd, u08* pval, u08 valLen){
	if ClkIsWrite(cmd){
		if (pval[0]<=ALARM_MAX){
			memcpy(ElementAlarm(pval[0]), (void*)pval, valLen);
			EeprmStartWrite();
		}
		EeprmStartWrite();										//Записать в eeprom
	}
}

/************************************************************************/
/* Установка громкости в модуле			                                */
/************************************************************************/
void espVolumeTx(enum tVolumeType Type){
	struct espVolume Vol;
	
	Vol.id = Type;
	Vol.Volume = VolumeClock[Type].Volume;
	Vol.State = 0;												//Для будильников состояние выключения не важно
	if (Type == vtButton)
		Vol.State = KeyBeepSettingIsSet();
	else if (Type == vtEachHour)
		Vol.State = EachHourSettingIsSet();
	Vol.LevelMetod = VolumeClock[Type].LevelVol;
	espUartTx(ClkWrite(CLK_VOLUME), (u08*) &Vol, sizeof(Vol));
}

/************************************************************************/
/* Принять команду управления громкостью                                */
/************************************************************************/
void UartVolumeSet(u08 cmd, u08* pval, u08 valLen){

	#define espVol	((struct espVolume*)pval)

	if ClkIsWrite(cmd){									//Запись значения
		VolumeClock[espVol->id].Volume = espVol->Volume;
		VolumeClock[espVol->id].LevelVol = espVol->LevelMetod;	//Тип регулирования громкости
		if (espVol->id == vtButton){					//Звук кнопок
			KeyBeepSettingSetOff();
			if (espVol->State)
				KeyBeepSettingSwitch();
		}
		else if(espVol->id == vtEachHour){				//Куранты
			EachHourSettingSetOff();					//Сначала выключаем
			if (espVol->State)
				EachHourSettingSwitch();				//Но надо все таки включить
		}
		EeprmStartWrite();								//Записать в eeprom
#ifdef DEBUG
SetTimerTask(espNetNameSet, 10000);
#endif
	}
	else if ClkIsTest(cmd){								//Тест громкости
		if (SoundIsBusy()){								//Прекратить звук если он звучит
			SoundOff();
			return;
		}
		struct sVolume Tmp=VolumeClock[pval[0]];		//Запомнить предыдущее стостояние
		u08 State = KeyBeepSettingIsSet()?1:0;
		u08 TestType = SND_TEST;						//По умолчанию звук кнопок тестируется
		VolumeClock[espVol->id].LevelVol = vlConst;		//Тип регулирования для всех константа
		VolumeClock[espVol->id].Volume = espVol->Volume;//Уровень громкости
		if(espVol->id == vtEachHour){					
			TestType = SND_TEST_EACH;
			State = EachHourSettingIsSet()?1:0;
			
		}
		else if (espVol->id == vtAlarm){
			TestType = SND_TEST_ALARM;
			State = 0;
			VolumeClock[espVol->id].LevelVol = espVol->LevelMetod;	//Для будильника свой
		}
		SoundOn(TestType);								//Звук и регулятор громкости запустить
		VolumeClock[espVol->id] = Tmp;					//Возвращаем старые значения
		struct espVolume espRet = *espVol;
		espRet.LevelMetod = Tmp.LevelVol;
		espRet.Volume = Tmp.Volume;
		espRet.State = State;
		espUartTx(ClkWrite(CLK_VOLUME), (u08*)&espRet, sizeof(espVol));
	}
}

/************************************************************************/
/* Передача значения сенсора											*/
/************************************************************************/
void espSendSensor(u08 numSensor){
	u08 cmd[sizeof(struct sSensor)+1];
	u08* cmd_ptr = cmd;
	
	memcpy((void*)(cmd_ptr+1), (void *)SensorNum(numSensor), sizeof(struct sSensor));		//Формируется команда записи датчика плюс его индекс
	cmd[0] = numSensor;
	espUartTx(ClkWrite(CLK_SENS), cmd, sizeof(cmd));
}

/************************************************************************/
/* Управление датчиками                                                 */
/************************************************************************/
void UartSensorSet(u08 cmd, u08* pval, u08 valLen){	

	if (ClkIsWrite(cmd)){
		if (pval[0]<=SENSOR_MAX){
			u08 ptr[sizeof(struct sSensor)+1];					//Промежуточный буфер
			memcpy((void*)ptr, (void*)pval, valLen);			//Заполнить буфер
			u08 *p = ptr;										//Указатель на первый элемент
			p++;
			memcpy(SensorNum(pval[0]), (void*)p, valLen-1);
			EeprmStartWrite();									//Записать в eeprom
		}
	}
	else if (ClkIsTest(cmd)){
		for(u08 i=0; i<=SENSOR_MAX;i++){	
			if (SensorNum(i)->Adr == pval[0]){
				CurrentSensorShow = i;
				SetClockStatus(csSensorSet, ssSensWaite);
				espSendSensor(i);
				break;
			}
		}
	}
}

/************************************************************************/
/* Передать в модуль текущее время                                      */
/************************************************************************/
void espWatchTx(void){
	espUartTx(ClkWrite(CLK_WATCH), (u08*) &Watch, sizeof(Watch));
}
/************************************************************************/
/* Принять из модуля время												*/
/************************************************************************/
void UartWatchSet(u08 cmd, u08* pval, u08 valLen){
	if (ClkIsWrite(cmd)){
		while(i2c_Do & i2c_Busy)						//Ожидается освобождение шины I2C
			TaskManager();
		memcpy((void*) &Watch, pval, sizeof(Watch));
		i2c_RTC_DirectWrt();
		if (RTC_IsNotValid())							//Был сбой питания, но теперь он преодолен
			RTC_ValidSet();								//Часы корректны
		if ((ClockStatus == csTune) && (SetStatus == ssTimeSet)){	//Если режим запроса времени из интеренета то сообщить об удачном получении времени
			SetClockStatus(csTune, ssTimeLoaded);
		}
	}
}

/************************************************************************/
/* Установить шрифт в часах                                             */
/************************************************************************/
void UartFontSet(u08 cmd, u08* pval, u08 valLen){
	if (*pval<FONT_COUNT_MAX){
		if (ClkIsWrite(cmd)){
			FontIdSet(*pval);
			SetClockStatus(csClock, ssNone);
			EeprmStartWrite();							//Записать в eeprom
		}
	}
}

/************************************************************************/
/* Передача в модуль имя сети и пароля для режима station с SD-карты    */
/************************************************************************/
void espNetNameSet(void){
	#define ESP_NET_VAL "netname="
	#define ESP_PAS_VAL "password="
	
	FRESULT Ret;
	WORD rb;												//Прочитанных байт
	char *NetName, *pass;
	u16 i;

	if NoSDCard() return;									//Нет sd карты - не с чем работать
	if espModuleIsNot() return;								//Модуля нет в системе
	
	if (SoundIsBusy()){
		SetTask(espNetNameSet);								//FatFs занята, попытаемся позже
		return;
	}	

	if (pf_disk_is_mount() != FR_OK){						//Карта еще не определялась, пробуем ее инициализировать.
		Ret = pf_mount(&fs);
		if (Ret != FR_OK) return;
	}
	Ret = pf_open("wifi.cfg");
	if (Ret == FR_OK){
		Ret = pf_lseek (0);
		if (Ret == FR_OK){
			Ret = pf_read(&read_buf0, BUF_LEN, &rb);
			if ((Ret == FR_OK) && (rb>(strlen(ESP_NET_VAL)+strlen(ESP_PAS_VAL)+4))){	//Прочитанных байт должно быть больше имени переменных
				*(read_buf0+rb+1) = 0;						//Конец строки
				for(i=0;i<BUF_LEN;i++){	//всякие переносы строки и прочее в 0
					if (*(read_buf0+i)<0x20){
						*(read_buf0+i) = 0;
					}
				}
				NetName = (void*)strstr((char*)read_buf0, ESP_NET_VAL);	//Имя сети
				if (NetName != NULL){
					NetName += strlen(ESP_NET_VAL);
					for(i=0; *(NetName+strlen(NetName)+i) == 0;i++);	//Пропускаем нули между строками
					pass = (void*)strstr(NetName+strlen(NetName)+i, ESP_PAS_VAL);//Пароль
					if (pass != NULL){
						pass += strlen(ESP_PAS_VAL);
						memset(read_buf1, 0, strlen(NetName)+strlen(pass)+8);
						memcpy(read_buf1, NetName, strlen(NetName));
						memcpy(read_buf1+strlen(NetName)+1, pass, strlen(pass));
//						espUartTx(ClkWrite(CLK_ST_WIFI_AUTH), read_buf1, strlen(NetName)+strlen(pass)+2);
						espUartTx(ClkRead(CLK_ST_WIFI_IP), read_buf1, strlen(NetName)+strlen(pass)+2);
					}
				}
			}
		}
	}
	pf_mount(0);
}

/************************************************************************/
/* Запрос IP адреса для режима station. Вынесено в процедуру для вызова */
/* из меню																*/
/************************************************************************/
void espGetIPStation(void){
	u08 i;
	espUartTx(ClkRead(CLK_ST_WIFI_IP), &i, 1);
}

/************************************************************************/
/* IP адрес получен                                                     */
/************************************************************************/
void UartStationIP(u08 cmd, u08* pval, u08 valLen){
	#define IP_LEN_STR (3*4+5)						//четыре октета по три знака плюс точки и нулевой байт

	if (espStationIP == NULL){
		espStationIP = malloc(IP_LEN_STR);
	}
	memcpy(espStationIP, pval, IP_LEN_STR);
	SetClockStatus(csTune, ssIP_Show);				//Вывести на дисплей
}

/************************************************************************/
/* Возврат в основной режим при получении команды CLC_STOP				*/
/************************************************************************/
void UartBaseModeSet(u08 cmd, u08* pval, u08 valLen){
	SetClockStatus(csClock, ssNone);
}

/************************************************************************/
/* Прочитать все настройки и счетчики  в часах                          */
/************************************************************************/
void UartGetAll(u08 cmd, u08* pval, u08 valLen){
	if ClkIsRead(cmd)
		espVarInit();
}
