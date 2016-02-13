/*
* Прием температуры от радиодатчика
*/

#include "nRF24L01P.h"
#include "EERTOS.h"
#include "sensors.h"
#include "Sound.h"

/************************************************************************/
/* Ввод- вывод байта по SPI. Возвращает значение прочитанное из MISO    */
/************************************************************************/
u08 nRF_ExchangeSPI(u08 Value){
	u08 Ret = 0;

	for(u08 i=8;i;i--){
		Ret <<= 1;
		if BitIsSet(nRF_PORTIN, nRF_MISO)				//Чтение MISO
			Ret |= 1;
		ClearBit(nRF_PORT, nRF_MOSI);
		if (Value & 0x80)								//Вывод MOSI
			SetBit(nRF_PORT, nRF_MOSI);
		Value <<= 1;
		SetBit(nRF_PORT, nRF_SCK);						//Записать бит
		ClearBit(nRF_PORT, nRF_SCK);
	}
	return Ret;
}

/************************************************************************/
/* Передача команды. Возвращает статус модуля							*/
/* cmd - код команды,													*/
/* Len - длина данных в буфере на который ссылается Data				*/
/************************************************************************/
u08 nRF_cmd_Write(const u08 cmd, u08 Len, u08 *Data){
	u08 Status;
	
	nRF_SELECT();
	Status = nRF_ExchangeSPI(cmd);
	if (Len){
		for(u08 i=0; Len; Len--, i++)
			nRF_ExchangeSPI(*(Data+i));
	}
	nRF_DESELECT();
	return Status;
}

/************************************************************************/
/* Прием байтов из nRF24L01P											*/
/* Len - количество байт требующих приема. 								*/
/* Возвращает последний принятый байт или статус модуля если len=0 (	*/
/* нулевом байте Buf так же будет статус модуля							*/
/************************************************************************/
u08 nRF_cmd_Read(const u08 Cmd, u08 Len, u08 *Data){
	u08 Ret;
	nRF_SELECT();
	Ret = nRF_ExchangeSPI(Cmd);
	if (Len){
		for(u08 i=0; Len; Len--, i++){
			*(Data+i) = nRF_ExchangeSPI(nRF_NOP);
			Ret = *(Data+i);
		}
	}
	else
		*Data = Ret;
	nRF_DESELECT();
	return Ret;
}
/************************************************************************/
/* Загрузка принятого ответа от радиодатчика                            */
/************************************************************************/
void nRF_Recive(void){
	u08 Buf[nRF_SEND_LEN], Status, CountByts;
	struct sSensor Sens;								//Ввел сюда структуру, что бы было видно, что здесь работаем со статусом датчика
	
	#define nRF_NoAnswer()	Sens.SleepPeriod = 0		//Нет ответа от передатчика или ответ не тот
	#define nRF_AnswerOk()	Sens.SleepPeriod = 0xff		//Получен ответ от передатчика
	#define nRF_AnswerIsOk() (Sens.SleepPeriod)
	
#ifdef nRF_IRQ											//Выведена нога прерывания
	if (nRF_NO_RECIV()){								//Ничего не принято
		SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
		return;
	}
#else													//Нога прерывания не доступна, нужен опрос модуля
	Status = nRF_cmd_Read(nRF_RD_REG(nRF_STATUS), 0, Buf);
	if ((Status & nRF_STAT_RESET) == 0){				//Ничего не принято
		SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
		return;
	}
#endif

	Sens.State = 0;
	SensorNoInBus(Sens);								//Ответа от радиодатчика еще нет
	Sens.Value = 0;										//Это что бы компилятор не выкидывал предупреждение
	nRF_NoAnswer();
	nRF_STOP();											//Остановить работу радиотракта

	Buf[0] = Status & nRF_STAT_RESET;					//сбросить флаги статуса
	nRF_cmd_Write(nRF_WR_REG(nRF_STATUS), 1, Buf);

	if nRF_RX_DATA_RDY(Status){							//Что-то принято

		CountByts = nRF_cmd_Read(nRF_RD_REG(nRF_R_RX_PL_WID), 1, Buf);
		if (CountByts > nRF_SEND_LEN)					//TODO: По непонятной причине принимается 11 байт вместо 4
			CountByts = nRF_SEND_LEN;
		
		if (CountByts == nRF_SEND_LEN){					//Размер пакета правильный
			nRF_AnswerOk();
			nRF_cmd_Read(nRF_R_RX_PAYLOAD, CountByts, Buf);
			nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);		//Очистить буфера FIFO
			
			if ((Buf[0] == nRF_RESERVED_BYTE) && (Buf[1] == nRF_TMPR_ATTNY13_SENSOR)){	//Правильный пакет
				SensorSetInBus(Sens);													//Есть ответ от радиодатчика
				if ((((u16)Buf[2]<<8) | ((u16)Buf[3])) == nRF_SENSOR_NO)				//Сенсор на шине не обнаружен
					Sens.Value = TMPR_NO_SENS;											//Сам датчик не отвечает
				else
					Sens.Value = ( ((Buf[2]<<4) & 0xf0) | ((Buf[3]>>4) & 0x0f) );		//Значение температуры
				if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite)			//Если режим ожидания именно этого датчика, то следующая передача через 1 секунду
					&& (SensorNum(CurrentSensorShow)->Adr == nRF_RX_PIP_READY_BIT_LEFT(Status))
					)
					{				
					Buf[0] = ATTINY_13A_1S_SLEEP;										//Команда для таймера сна
					Buf[1] = 0x00;														//Старший байт счетчика сна
					Buf[2] = nRF_MEASURE_TEST_SEC;										//Младший байт счетчика
				}
				else{																	//Если нормальный режим то следующая передача через 20 минут	
					Buf[0] = ATTINY_13A_8S_SLEEP;										//Команда для таймера сна
					Buf[1] = nRF_INTERVAL_NORM_MSB;										//Старший байт счетчика сна
					Buf[2] = nRF_INTERVAL_NORM_LSB;										//Младший байт счетчика
				}
				nRF_cmd_Write(nRF_FLUSH_TX, 0, Buf);									//Очистить буфер передачи
				nRF_cmd_Write((nRF_W_ACK_PAYLOAD_MASK | (nRF_RX_PIP_READY_BIT_LEFT(Status)>>1)), nRF_ACK_LEN, Buf);	//Записать ответ для канала
			}
		}
		else{																			//Принят неправильный пакет, очищаем буфер приема
			nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);
		}
	}

	if nRF_AnswerIsOk(){
		SetSensor(nRF_RX_PIP_READY_BIT_LEFT(Status), Sens.State, Sens.Value);	//Записать данные в массив датчиков. Адрес датчика сдвинут. Подробнее см. в объявлении SetSensor
		if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite)){		//Если режим ожидания датчика, то сразу обновить экран
			Refresh();
		}
	}

	SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
	nRF_GO();											//Продолжить прием
}

/************************************************************************/
/* Инициализация nRF24L01												*/
/************************************************************************/
void nRF_Init(void){
	u08 Buf[nRF_ACK_LEN], Status;

	nRF_DDR |=	(1<<nRF_MOSI)	|
				(0<<nRF_MISO)	|
				(1<<nRF_SCK)	|
				#ifdef nRF_CE
				(1<<nRF_CE)		|						//Запуск обмена
				#endif
				#ifdef nRF_IRQ
				(1<<nRF_IRQ)	|
				#endif
				(1<<nRF_CSN);							//Выбор модуля
	nRF_PORT |= (1<<nRF_MISO);							//Подтянуть вход к питанию на всякий случай

	nRF_STOP();											//Радио выключено
	nRF_DESELECT();										//Модуль не выбран
	nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);				//Проверить наличие модуля на SPI. Сначала Очистить буфера RX и TX FIFO
	nRF_cmd_Write(nRF_FLUSH_TX, 0, Buf);
	Status = nRF_cmd_Read(nRF_WR_REG(nRF_STATUS), 0, Buf);

	Buf[0] &=	(1<<nRF_STATUS_BIT7) |					//Этот бит должен быть всегда 0
				(1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0) | //Эти биты после очистки буфера RX FIFO должны быть равны 111 (буфер RX FIFO пуст)
				(1<<nRF_TX_FULL_ST);					//После очистки буфера TX FIFO должен быть равен 0 (в буфере TX FIFO есть место)
	
	if (Buf[0] != ((0<<nRF_STATUS_BIT7) |				//Этот бит должен быть всегда 0
				(1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0) | //Эти биты после очистки буфера RX FIFO должны быть равны 111 (буфер RX FIFO пуст)
				(0<<nRF_TX_FULL_ST))					//После очистки буфера TX FIFO должен быть равен 0 (в буфере TX FIFO есть место)
		){												//Если это не так следовательно модуля нет в составе системы
		return;											//Модуля нет, никаких действий не производим
	}

	Buf[0] = Status & nRF_STAT_RESET;					//сбросить флаги прерываний на всякий случай
	nRF_cmd_Write(nRF_WR_REG(nRF_STATUS), 1, Buf);
	
	Buf[0] = (1<<nRF_ERX_P5) | (1<<nRF_ERX_P4) | (1<<nRF_ERX_P3) | (1<<nRF_ERX_P2) | (1<<nRF_ERX_P1) | (1<<nRF_ERX_P0);
	nRF_cmd_Write(nRF_WR_REG(nRF_EN_RXADDR), 1, Buf);	//разрешить адресовать указанные выше каналы

	Buf[0] = (1<<nRF_EN_DPL) | (1<<nRF_EN_ACK_PAY);		//Разрешить динамическую длину пакетов и выдачу ответа
	nRF_cmd_Write(nRF_WR_REG(nRF_FEATURE), 1, Buf);

	Buf[0] = (1<<nRF_DPL_P5) | (1<<nRF_DPL_P4) | (1<<nRF_DPL_P3) | (1<<nRF_DPL_P2) | (1<<nRF_DPL_P1) | (1<<nRF_DPL_P0);//Разрешить динамическую длину пакетов для всех каналов
	nRF_cmd_Write(nRF_WR_REG(nRF_DYNPD), 1, Buf);

	Buf[0] = (1<<nRF_PRIM_RX) | (1<<nRF_PWR_UP) | (1<<nRF_EN_CRC);	//Режим приема
	nRF_cmd_Write(nRF_WR_REG(nRF_CONFIG), 1, Buf);

	Buf[0] = ATTINY_13A_1S_SLEEP;						//Команда для таймера сна
	Buf[1] = nRF_INTERVAL_NORM_MSB;						//Старший байт счетчика сна
	Buf[2] = nRF_INTERVAL_NORM_LSB;						//Младший байт счетчика
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);	//Записать ответ для канала nRF_PIPE
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);

	SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);			//Включить опрос модуля
	nRF_GO();											//Начать прием
}