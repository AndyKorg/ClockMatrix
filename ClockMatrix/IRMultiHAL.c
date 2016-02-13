/*
 * ver. 0.1
 */ 

#include "IRMultiHAL.h"
#include "eeprom.h"

#ifndef IR_SAMSUNG_ONLY

#define IR_PULSE_PERIOD		1152000							//Минимальный период одного бита, 1,152 ms = 1152 us = 1152000
#define IR_RELOAD_PERIOD	19200000						//Максимальная длительность пакета, us

#define IR_PULSE_THERSHOLD	(IR_PULSE_PERIOD/PERIOD_INT_IR)	//Порог длительности импульса по которому определяется 1 или 0
#if (IR_PULSE_THERSHOLD > 65536)
	#error "IR_PULSE_THERSHOLD is greater then 65536"
#endif

#define IR_RELOAD			(IR_RELOAD_PERIOD/PERIOD_INT_IR)
#if (IR_RELOAD > 65536)
	#error "IR_RELOAD is greater 65536"
#endif

#define IR_DELAY_PROTECT	100								//Защитный интервал между ИК-пакетами

volatile struct ir_t
{
	uint8_t rx_started;										//Флаг занятости автомата
	uint32_t rx_buffer;										//Буфер пакета
} Ir;


//Произошло изменение уровня на ноге, определяем длительность импульса
ISR(IR_INT_VECT)
{
	if(Ir.rx_started){										//Идет прием
		Ir.rx_buffer <<= 1;									//Подготовить разряд в буфере приема
		if(PeriodIR >= IR_PULSE_THERSHOLD)					//Если длительность импульса больше порога то это считается единицей
			Ir.rx_buffer |= 1;
	}
	else
		Ir.rx_started = 1;									//Старт приема
	PeriodIR = 0;
}


/************************************************************************/
/* Команда с ИК пульта готова. Вынесена в отдельную задачу, что бы      */
/* освободить прерывание											    */
/************************************************************************/
void IRReadyCommand(void){
	if (IRReciveReady != NULL){								//Есть функция обработки принятой посылки, вызываем ее
		IRReciveReady(REMOT_TV_ADR, Ir.rx_buffer);			//В случае универсального пульта все команды с ИК приемника считаются командами ДУ и никак иначе.
	}
	Ir.rx_buffer = 0;
}

/************************************************************************/
/* Вызывается с периодом PERIOD_INT_IR                                  */
/************************************************************************/
void IRMultiInc(void){
	if (Ir.rx_started){
		if (PeriodIR > IR_RELOAD){							//Сигналов с пульта не было более IR_RELOAD, это считается концом пакета
			StopIRCounting();								//Стоп отсчета до окончания обработки команды
			Ir.rx_started = 0;								
			SetTask(IRReadyCommand);						//поставить команду на обработку
		}
		else
			PeriodIR++;
	}
}

/************************************************************************/
/* Инициализация массива кодов клавиш и старт приема                    */
/************************************************************************/
void IRRecivHALInit(void){
	//Восстанавливаются коды из eeprom. Если кода нет то восстанавливаются коды для пульта samsung
	if (EeprmReadIRCode(IR_OK) == EEP_BAD)
		IRcmdList[IR_OK] =			0xAA0800A2;	//Ok			
	if (EeprmReadIRCode(IR_STEP) == EEP_BAD)
		IRcmdList[IR_STEP] =		0x2280882A;	//Menu
	if (EeprmReadIRCode(IR_PLAY_START) == EEP_BAD)
		IRcmdList[IR_PLAY_START] =	0xA80802A2;	//воспроизведение
	if (EeprmReadIRCode(IR_PLAY_STOP) == EEP_BAD)
		IRcmdList[IR_PLAY_STOP] =	0x280882A2; //стоп
	if (EeprmReadIRCode(IR_INC) == EEP_BAD)
		IRcmdList[IR_INC] =			0x20808A2A; //Канал+
	if (EeprmReadIRCode(IR_DEC) == EEP_BAD)
		IRcmdList[IR_DEC] =			0x0080AA2A;	//Канал-
	
	Ir.rx_started = 0;										//автомат еще не начал работу
	Ir.rx_buffer = 0;
	PeriodIR = 0;
	IRPeriodEvent = IRMultiInc;								//Обработка события происходящего каждые 30.5 мс
	IntIRUpDownFront();										//Любое изменение уровня на ноге вызывает прерывание
	StartIRCounting();										//Разрешить прерывания - старт автомата
}

#endif	//#ifndef IR_SAMSUNG_ONLY
