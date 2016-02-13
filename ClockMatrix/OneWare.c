/*
 * OneWare.c
 * Обслуживание на шине 1-Ware с помощью прерываний.
 * На время чтения бита блокируются внешнеи прерывания поскольку они имеют более высокий приоритет
 * чем прерывания от таймера. Время блокировки составляет OW_REPEAT_END_DELAY()+OW_START_SLOT_DELAY() мкс.
 * И все равно работа шины не стабильна. 
 * Поэтому наверно нужно использовать например блок USART для шины 1-ware.
 * ver 1.2
 */ 

#include "OneWare.h"
#include <util/delay.h>

//------------- Описание порта 1-Ware
#define OW_DDR					DDRD
#define OW_PORT_OUT				PORTD
#define OW_PORT_IN				PIND
#define OW_PIN					PORTD6
#define OW_INI()				ClearBit(OW_PORT_OUT, OW_PIN)		//Записать вывод 0 для прижатия к земле
#define OW_NULL()				SetBit(OW_DDR, OW_PIN)				//Прижать к земле
#define OW_ONE()				ClearBit(OW_DDR, OW_PIN)			//Отпустить
#define OW_IS_NULL()			BitIsClear(OW_PORT_IN, OW_PIN)		//На входе низкий уровень
#define OW_IS_ONE()				BitIsSet(OW_PORT_IN, OW_PIN)		//На входе высокий уровень


//------------- Настройка таймера
#define OW_TCNT					TCNT2								//Сам счетчик
#define OW_TCCR					TCCR2								
#ifndef PSR2
#define PSR2					1									//Почему-то этот бит не описан в io.h
#endif
#define OW_PRE_RESET()			SetBit(SFIOR, PSR2)					//Сброс предделителя именно 2-ого счетчика, причем этот предделитель обслуживает только один таймер номер 2
#define OW_ENABLE_INT()			SetBit(TIMSK, TOIE2)				//Разрешить прерывание по переполнению
#define OW_DISABLE_INT()		ClearBit(TIMSK, TOIE2)				//Запретить прерывание по переполнению
#define OW_DELAY_INT			TIMER2_OVF_vect						//Таймер обеспечивающий отсчет задержек
#define OW_INTERRAPT			TOIE2								//Флаг разрешения прерывания
#define OW_DELAY_INIT			(0<<FOC2) | (0<<WGM20) | (0<<WGM21) | (0<<COM21) | (0<<COM20) //ПРИ СМЕНЕ ТАЙМЕРА ЗАМЕНИТЬ НАЗВАНИЯ БИТОВ РЕЖИМА!
#define OW_CS0					CS20								//Биты управления предделтелем
#define OW_CS1					CS21
#define OW_CS2					CS22
#define OW_DELAY_STOP()			OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//Остановить счетчик
//#define OW_DELAY_STOP()			do {OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0); OW_DISABLE_INT();}while(0)	//Остановить счетчик
#define OW_PRE64()				OW_TCCR = OW_DELAY_INIT | (1<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//Предделитель на 64
#define OW_PRE32()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (1<<OW_CS0)	//Предделитель на 32
#define OW_PRE8()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (0<<OW_CS0)	//Предделитель на 8
#define OW_PRE1()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (1<<OW_CS0)	//Предделитель на 1
#define OW_BORDER				255									//Граница от которой надо отсчитывать задержки
#define OW_DELAY_VALUE(dly, div) (OW_BORDER-(F_CPU/div)/(1000000UL/dly))	//Расчет значения счетчика для конкретной задержки для конкретно предделителя
#define OW_DELAY_64(delay)	do{OW_PRE64();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 64);}while(0)
#define OW_DELAY_32(delay)	do{OW_PRE32();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 32);}while(0)
#define OW_DELAY_8(delay)	do{OW_PRE8();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 8);}while(0)
#define OW_DELAY_1(delay)	do{OW_PRE1();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 1);}while(0)
//------------- Задержки для тайм-слота
//Длительность импульса Reset на шине 1-Ware 480 мкс минимум. 
#define OW_RESET_DELAY_us			480
#define OW_RESET_PRE				64
#if (OW_RESET_PRE == 64)
	#define OW_RESET_DELAY()		OW_DELAY_64(OW_RESET_DELAY_us)			
#elif (OW_RESET_PRE == 32)
	#define OW_RESET_DELAY()		OW_DELAY_32(OW_RESET_DELAY_us)
#elif (OW_RESET_PRE == 1)
	#define OW_RESET_DELAY()		OW_DELAY_1(OW_RESET_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) >= 255)		//Проверка получившего значения счетчика для выбранного предделителя
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//Старт тайм-слота
#define OW_START_SLOT_DELAY_us		2
#define OW_START_SLOT_DELAY_PRE		1
#if (OW_START_SLOT_DELAY_PRE == 64)
	#define OW_START_SLOT_DELAY()		OW_DELAY_64(OW_START_SLOT_DELAY_us)
#elif (OW_START_SLOT_DELAY_PRE == 32)
	#define OW_START_SLOT_DELAY()		OW_DELAY_32(OW_START_SLOT_DELAY_us)
#elif (OW_START_SLOT_DELAY_PRE == 1)
	#define OW_START_SLOT_DELAY()		OW_DELAY_1(OW_START_SLOT_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_START_SLOT_DELAY_us, OW_START_SLOT_DELAY_PRE) >= 255)	//Проверка получившего значения счетчика для выбранного предделителя
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_START_SLOT_DELAY_us, OW_START_SLOT_DELAY_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//Длительность тайм-слота
#define OW_MAX_SLOT_us				60
#define OW_MAX_SLOT_PRE				32
#if (OW_MAX_SLOT_PRE == 64)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_64(OW_MAX_SLOT_us)
#elif (OW_MAX_SLOT_PRE == 32)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_32(OW_MAX_SLOT_us)
#elif (OW_MAX_SLOT_PRE == 1)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_1(OW_MAX_SLOT_us)
#endif
#if (OW_DELAY_VALUE(OW_MAX_SLOT_us, OW_MAX_SLOT_PRE) >= 255)
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_MAX_SLOT_us, OW_MAX_SLOT_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//Промежуток между тайм-слотами
#define OW_INTERVAL_SLOT_DELAY_us	10
#define OW_INTERVAL_SLOT_DELAY_PRE	1
#if (OW_INTERVAL_SLOT_DELAY_PRE == 64)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_64(OW_INTERVAL_SLOT_DELAY_us)
#elif (OW_INTERVAL_SLOT_DELAY_PRE == 32)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_32(OW_INTERVAL_SLOT_DELAY_us)
#elif (OW_INTERVAL_SLOT_DELAY_PRE == 1)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_1(OW_INTERVAL_SLOT_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_INTERVAL_SLOT_DELAY_us, OW_INTERVAL_SLOT_DELAY_PRE) >= 255)	//Проверка получившего значения счетчика для выбранного предделителя
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_INTERVAL_SLOT_DELAY_us, OW_INTERVAL_SLOT_DELAY_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//Задержка после выдачи старта тайм-слота для анализа ответного бита от устройства
#define OW_REPEAT_END_us			2//9
#define OW_REPEAT_END_PRE			1
#if (OW_REPEAT_END_PRE == 64)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_64(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 32)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_32(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 8)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_8(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 1)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_1(OW_REPEAT_END_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_REPEAT_END_us, OW_REPEAT_END_PRE) >= 255)
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_REPEAT_END_us, OW_REPEAT_END_PRE) <= 2)
	#error "Prescaller less 2"
#endif

#define OW_STEP_2_US				2								//Значения 2 микросекунд, определено для облегчения понимания кода
#define OW_MAX_WAIT_RESET_US		240								//Значения 240 микросекунд, максимальная длительность ожидания ответа от устройства на импульс reset
#define OW_BIT_IN_BYTE				8								//Количество отсчетов в байте

//------------ Объявления функций ------------------------------
void owResetBus(u08 Reset);

typedef void (*OW_EVNT)(u08 Reset);									//тип указателя на функцию обработки события окончания периода
#define OW_DELAY_RESET				1								//Сбросить счетчик состояния функции OW_EVNT
#define OW_DELAY_NORMAL				0								//Нормальная работа функции OW_EVNT в соответствии с внутренним счетчиком шагов
volatile OW_EVNT	EndDelayEvent;									//функция вызываемая в конце задержки

#define ONE_WARE_BUF_LEN			2								//Размер передаваемых и принимаемых данных
volatile u08	owStatus,											//Текущий статус шины 1-Ware, флаговая переменная
				owRecivBuf[ONE_WARE_BUF_LEN],						//Буфер для принятых байт
				owSendBuf[ONE_WARE_BUF_LEN];						//Буфер для передаваемых байт

/************************************************************************/
/* Прием байта															*/
/************************************************************************/
void owRecivData(u08 Reset){
	static u08 State, ByteCount, BitCount, ByteBuf, TmpGICR;
	
	if (Reset){														//Сбросить внутреннее состояние автомата
		State = 0;
		ByteCount = 0;
		BitCount = OW_BIT_IN_BYTE;
		ByteBuf = 0;
		return;
	}
	switch (State){
		case 0:														//Передать старт тайм-слота
			OW_NULL();												//Начать передачу старта тайм-слота
			TmpGICR = GICR;
			GICR &= ~((1<<INT0) | (1<<INT1) | (1<<INT2));
//			_delay_us(3);
			OW_START_SLOT_DELAY();									//6 us
			State++;
			break;
		case 1:
			OW_ONE();
//			_delay_us(15);											//Попытка замены этого интервала отсчетом таймера не увеначалась успехом. Чтение стало нестабильным
			OW_REPEAT_END_DELAY();									//9 us
			State++;
			break;
		case 2:
			BitCount--;
			ByteBuf >>= 1;											//Следующий бит
			if OW_IS_ONE()
				SetBit(ByteBuf, 7);
			GICR = TmpGICR;
			OW_MAX_SLOT_DELAY();									//60 us
			if (BitCount == 0){										//Байт принят?
				BitCount = OW_BIT_IN_BYTE;
				owRecivBuf[ByteCount] = ByteBuf;					//Запомнить его в буфере
				ByteBuf = 0;
				ByteCount++;										//Следующий байт
				if (ByteCount == ONE_WARE_BUF_LEN){					//Весь пакет принят
					State++;
					break;
				}
			}
			State = 0;												//Начать с передачи старта тайм-слота
			break;
		case 3:														//Прием байта завершен, обеспечивается задержка между слотами
			OW_INT_SLOT_DELAY();
			State++;
			break;
		case 4:														//Конец передачи
			OW_DELAY_STOP();										//Таймер останавливается
			owFree();												//Шина свободна
			owCompleted();											//Вызывается функция по окончании передачи
			break;
		default:
			break;
	}
}

/************************************************************************/
/* Передача байта														*/
/************************************************************************/
void owSendData(u08 Reset){
	static u08 State, ByteCount, BitCount, ByteBuf;
	
	if (Reset == OW_DELAY_RESET){									//Сбросить внутреннее состояние автомата
		State = 0;
		ByteCount = 0;
		BitCount = OW_BIT_IN_BYTE;
		ByteBuf = owSendBuf[0];
		return;
	}
	switch (State){
		case 0:														//Передать старт тайм-слота
			OW_NULL();												//Начать передачу старта тайм-слота
			OW_START_SLOT_DELAY();									//6 us
			State++;
			break;
		case 1:
//			_delay_us(2);
			if (ByteBuf & 1)										//В зависимости от бита прижать или отпустить шину на 60 мкс
				OW_ONE();
			OW_MAX_SLOT_DELAY();									//Тайм слот бита 60 us
			State++;
			break;
		case 2:														//Передать промежуток между тайм-слотами
			OW_ONE();												//Отпустить шину
			OW_INT_SLOT_DELAY();									//10 us
			BitCount--;
			if (BitCount){											//Есть биты для передачи?
				ByteBuf >>= 1;										//Следующий бит
			}
			else{													//Байт передан, берется следующий
				ByteCount++;
				BitCount = OW_BIT_IN_BYTE;
				if (ByteCount == ONE_WARE_BUF_LEN){					//Весь пакет передан
					if owIsSndAndRcv(){								//Надо еще прочитать данные из устройства?
						owRecivStat();								//Переключить автомат в режим приема
						owResetBus(OW_DELAY_RESET);					//Подготовить автомат для генерации сброса на шине
						EndDelayEvent = owResetBus;
					}
					else{											//Больше не требуется ничего делать
						State++;
						break;
					}
				}
				else												//Следующий байт
					ByteBuf = owSendBuf[ByteCount];
			}
			State = 0;												//Следующий бит
			break;
		case 3:														//Конец передачи
			OW_INT_SLOT_DELAY();									//10 us
			State++;
			break;
		case 4:														//Конец передачи
			OW_DELAY_STOP();										//Таймер останавливается
			owFree();												//Шина свободна
			owCompleted();											//Вызывается функция по окончании передачи
			break;
		default:
			break;
	}
}

/************************************************************************/
/* Сброс шины и ожидание ответа от устройств на шине                    */
/************************************************************************/
void owResetBus(u08 Reset){
	static u08 State;
	
	if (Reset == OW_DELAY_RESET){									//Сбросить внутреннее состояние автомата
		State = 0;
		return;
	}
	if (State == 0){												//Окончание импульса сброса, начало защитного интервала
		OW_ONE();													//Отпустить шину
		OW_REPEAT_END_DELAY();
	}
	else if((State >=1) && (State<=(OW_MAX_WAIT_RESET_US/OW_STEP_2_US))){ //Проверка шины на появление низкого уровня с периодичностью 2 мкс в течении 240 мкс
		OW_INT_SLOT_DELAY();										//Этот интервал неточно отслеживается поскольку не слишком короткий, но оставлю пока так.
		if OW_IS_NULL(){											//Ответ обнаружен, можно передавать команды
			owDeviceSet();											//Устройство обнаружено
			OW_RESET_DELAY();										//Подождем еще 480 мкс для окончания импульса PRESENCE
			State = OW_MAX_WAIT_RESET_US/OW_STEP_2_US;				//Прекратить опрос шины
		}
	}
	else{															//Ожидание ответа от устройства закончено, переходим к обработке результатов
		if owDeviceIsNo(){											//Если устройство не обнаружено на шине
			OW_DELAY_STOP();										//Нет устройства освобождаем шину и вызываем функцию ошибки
			ClearBit(TIMSK, OW_INTERRAPT);							//Запретить прерывания от таймера
			owFree();												//Освободить шину
			owError();												//Вызвать функцию ошибки
			return;
		}
		EndDelayEvent = owIsRecivStat()? owRecivData : owSendData;	//В зависимости от режима дальше будет работать соответствующая процедура
		EndDelayEvent(OW_DELAY_RESET);								//Подготовить работу функции
		OW_INT_SLOT_DELAY();										//Начать выполнение операции через короткий промежуток
	}
	State++;
}

ISR(OW_DELAY_INT){
	EndDelayEvent(OW_DELAY_NORMAL);
}

/************************************************************************/
/* Начать обмен по шине 1-Ware											*/
/************************************************************************/
void owExchage(void){
	if owIsBusy(){													//Если шина занята то ошибка обращения к шине
		owError();
		return;
	}

	ClearBit(OW_DDR, OW_PIN);										//Порт на ввод - это на всякий случай
	SetBit(OW_PORT_OUT, OW_PIN);									//Включить подтягивающий резистор

	EndDelayEvent = owResetBus;
	EndDelayEvent(OW_DELAY_RESET);									//Подготовить работу функции события окончания задержки
	owBusy();														//Занять шину
	owDeviceNo();													//Считать что устройства пока нет
	owSendStat();													//Режим передачи

	OW_NULL();														//Начало ресет-импульса
	ClearBit(OW_PORT_OUT, OW_PIN);									//Прижать шину к земле. Так то по умолчанию тут 0, но это на всякий случай
	
	OW_RESET_DELAY();
	OW_ENABLE_INT();												//Разрешить прерывание
}
