/*
 *	Декодирование команд с ИК-приемника
 * описание протокола для пульта Samsung http://www.techdesign.be/projects/011/011_waves.htm и http://rusticengineering.com/2011/02/09/infrared-room-control-with-samsung-ir-protocol/
 * используются прерывания INT0 - для обнаружения фронтов и спадов импульсов и INT1 (совместно с дисплеем) для отсчета 30,5 мс интервалов
 * Кроме того приемник ИК выдает инверсный сигнал по сравнению со стандартом
 * Этот же модуль используется для обработки сигналов от внешних датчиков температуры и давления.
 * ver 1.2
 */ 

#include "IRreciveHAL.h"
#include "EERTOS.h"

#ifdef IR_SAMSUNG_ONLY		//<<<<<<<<-------------------------------- Вырезается если не используется протокол Samsung
/*
 *	Коды клавиш пульта samsung AA59-00793A от телевизора Samsung Smart-TV
 */
#define REMOT_PLAY_START	0xe2										//начать воспроизведение (кнопка воспроизведение)
#define REMOT_PLAY_STOP		0x62										//остановить воспроизведение (кнопка стоп)
#define REMOT_MENU_STEP		0x58										//аналог кнопки STEP на клавиатуре (кнопка MENU)
#define REMOT_MENU_OK		0xf2										//аналог OK на клавиатруе (кнопка GUIDE)
#define REMOT_CHANAL_UP		0x48										//Увеличить номер канала - здесь увеличить число в счетчике
#define REMOT_CHANAL_DOWN	0x08										//Обратное действие

#define REMOT_0				0x88										//0
#define REMOT_1				0x20										//1
#define REMOT_2				0xa0										//2
#define REMOT_3				0x60										//3
#define REMOT_4				0x10										//4
#define REMOT_5				0x90										//5
#define REMOT_6				0x50										//6
#define REMOT_7				0x30										//7
#define REMOT_8				0xb0										//8
#define REMOT_9				0x70										//9

//------------------ Периоды протокола Samsung, ns. В наносекундах что бы точность повысить
#define StartBitPeriodIR	((u08) (4500000/PERIOD_INT_IR))	//Нижняя граница длительности старт бита, 4.5 мс = 4500 000 ns
#define BeginBit0PeriodIR	((u08) (560000/PERIOD_INT_IR))	//Нижняя граница длительности начала бита и конца бита со значением ноль и стоп-бита, 0,56 мс = 560 000 ns
#define EndBit1PeriodIR		((u08) (1690000/PERIOD_INT_IR))	//Нижняя граница длительности конца бита со значением единица, 1,69 мс
#define Period4500IsCorrect	((PeriodIR >= (StartBitPeriodIR-2)) && (PeriodIR <= (StartBitPeriodIR+3))) //Период попадает в интервал 4500 мкс
#define Period560IsCorrect	((PeriodIR >= (BeginBit0PeriodIR-2)) && (PeriodIR <= (BeginBit0PeriodIR+3))) //Период попадает в интервал 560 мкс
#define Period1690IsCorrect	((PeriodIR >= (EndBit1PeriodIR-2)) && (PeriodIR <= (EndBit1PeriodIR+3))) //Период попадает в интервал 1690 мкс

#define PeriodMinimum		(560000/PERIOD_INT_IR/2)		//Минимальная длительность импульсов для анализа

volatile u08  StatusIR;										//Состояние приемника
			 
//----------------- Значения переменной StatusIR
#define StatIRStartBit1		0								//Обнаружение стартбита первый полупериод
#define StatIRStartBit2		1								//Обнаружение cтартбита второй полупериод
#define StatIRStartBitOK	2								//Принят спад конца стопбита
#define StatIRBeginBit		3								//Обнаружено начало-конец бита данных
#define StatIREndBit		4								//Обнаружена середина интервала бита данных

volatile u08 CommandIR, AdresIR;							//Принятые адрес и данные

/************************************************************************/
/* Преобразование кода принятой клавиши цифре							*/
/* возвращает 10 если код не соответствует цифре						*/
/************************************************************************/
u08 ConvertIRCodeToDec(u08 Code){
	switch (Code){
		case REMOT_0:								//Кнопки цифр, используются только для настройки счетчиков в режима настройки часов или будильников
			return 0;
			break;
		case REMOT_1:
			return 1;
			break;
		case REMOT_2:
			return 2;
			break;
		case REMOT_3:
			return 3;
			break;
		case REMOT_4:
			return 4;
			break;
		case REMOT_5:
			return 5;
			break;
		case REMOT_6:
			return 6;
			break;
		case REMOT_7:
			return 7;
			break;
		case REMOT_8:
			return 8;
			break;
		case REMOT_9:
			return 9;
			break;
		default:
			return 10;
			break;
	}
}

/************************************************************************/
/* сброс автомата расшифровки посылок                                   */
/************************************************************************/
inline void ResetIR(void){
IntiRDownFront();											//Обнаруживать спадающий фронт
StatusIR = StatIRStartBit1;									//Начальная точка автомата
}

/************************************************************************/
/* Функция вызываемая с периодом PERIOD_INT_IR                          */
/************************************************************************/
void IRPeriodAdd(void){
	PeriodIR++;
}

/************************************************************************/
/* Инициализация приемника ИК                                           */
/************************************************************************/
void IRRecivHALInit(void){

IRcmdList[IR_OK] = REMOT_MENU_OK;							//Заполнить массив команд кодами для пульта samsung
IRcmdList[IR_STEP] = REMOT_MENU_STEP;
IRcmdList[IR_PLAY_START] = REMOT_PLAY_START;
IRcmdList[IR_PLAY_STOP] = REMOT_PLAY_STOP;
IRcmdList[IR_INC] = REMOT_CHANAL_UP;
IRcmdList[IR_DEC] = REMOT_CHANAL_DOWN;
IRPeriodEvent = IRPeriodAdd;
ResetIR();
StartIRCounting();											//Старт ожидания сигнала от ИК приемника
}

/************************************************************************/
/* Обработка принятого кода по ИК каналу                                */
/************************************************************************/
void IRReadyCommand(void){
	if (IRReciveReady != NULL)								//Есть функция обработки принятой посылки, вызываем ее
		IRReciveReady(AdresIR, CommandIR);
}

/************************************************************************/
/* Обнаружение фронтов и спадов импульсов от ИК-приемника               */
/************************************************************************/
ISR(IR_INT_VECT){
	static u08 RecivByte, /*CommandIR, AdresIR, */NumBit, NumByte;
	
	if ((StatusIR != StatIRStartBit1) && (PeriodIR <= PeriodMinimum))	//Игнорировать слишком короткие импульсы после начала приема стартового перепада
		return;
		
	switch (StatusIR){
		case StatIRStartBit1:								//Обнаружен спадающий фронт возможно стартбита
			PeriodIR = 0;
			StatusIR = StatIRStartBit2;						//Ждем обнаружения нарастающего фронта
			IntIRUpFront();
			break;
		case StatIRStartBit2:								//Нарастающий фронт первого полупериода старт бита
			if Period4500IsCorrect{							//Длительность между спадом и нарастанием равна длительности полупериода старт бита?
				PeriodIR = 0;
				IntiRDownFront();
				StatusIR = StatIRStartBitOK;				//ждем окончания второго полупериода стартбита и начала данных
			}
			else
				ResetIR();
			break;
		case StatIRStartBitOK:								//Возможно принят весь старт бит
			if Period4500IsCorrect{							//Длительность между нарастанием и спадом равна длительности полупериода старт бита?
				PeriodIR = 0;
				IntIRUpFront();
				StatusIR = StatIREndBit;					//Ожидается конец интервала начала бита данных
				RecivByte = 0;
				AdresIR = 0;
				NumBit = 0;
				NumByte = 0;
			}
			else
				ResetIR();
			break;
		case StatIRBeginBit:								//Принят спад начала интервала бита данных
			IntIRUpFront();
			StatusIR = StatIREndBit;						//Следующий бит
			if Period560IsCorrect{							//Втора половина интервала данных длинной нулевого бита, записываем его
				RecivByte = (RecivByte<<1) & 0xfe;
			}
			else if Period1690IsCorrect
			{												//Вторая половина интервала данных длинной единичного бита, записываем его
				RecivByte = (RecivByte<<1) | 0x1;
			}
			else{											//Не та длительность, какая-то ошибка
				ResetIR();
				return;
			}
			NumBit++;
			if (NumBit == 8){								//Принят байт?
				switch (NumByte){
					case 0:									//Принят первый адрес
						AdresIR = RecivByte;
						break;
					case 1:									//Принят весь адрес проверяем
						if (RecivByte != AdresIR){			//Ошибка приема?
							ResetIR();
							return;
						}
						break;
					case 2:									//Принят прямой байт данных
						CommandIR = RecivByte;
						break;
					case 3:									//Принят инверсный байт данных, проверяем
						if ((RecivByte & CommandIR) == 0){	//Все нормально, посылка принята нормально
							StopIRCounting();				//Выключаем прием команд пока не будет обработана текущая
							SetTask(IRReadyCommand);		//Ставим в очередь готовую команду
						}
						ResetIR();
						return;
					default:
						break;
				}
				NumByte++;									//Следующий байт
				NumBit = 0;
			}
			PeriodIR = 0;
			break;
		case StatIREndBit:									//Принят подъем интервала начала бита данных
			if Period560IsCorrect{							//Должно быть фиксированной величины 
				PeriodIR = 0;
				IntiRDownFront();
				StatusIR = StatIRBeginBit;					//Начинаем прием интервала конца бита. Его период и определяет что принимается - еденица или ноль
			}
			else											//Не та длительность, какая-то ошибка
				ResetIR();
		default:
			break;
	}
}

#endif	//IR_SAMSUNG_ONLY
