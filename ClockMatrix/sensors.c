/*
 * sensors.c
 * ver 1.3
 */ 

#include <stddef.h>
#include "sensors.h"
#include "pff.h"
#include "eeprom.h"
#include "Sound.h"
#include "esp8266hal.h"

struct sSensor Sensors[SENSOR_MAX];

/************************************************************************/
/* Возвращает указатель на сенсор id									*/
/************************************************************************/
struct sSensor *SensorNum(u08 id){
	if (id<SENSOR_MAX)
		return &Sensors[id];
	else
		return &Sensors[0];
}


/************************************************************************/
/* Инициализирует структуру описания сенсоров из файла на SD-карте		*/
/************************************************************************/
void SensorIni(void){
	u08 i = 0;
	
	for(;i<SENSOR_MAX;i++){
		Sensors[i].Name[0] = 'д';
		Sensors[i].Name[1] = 0x30 | i;								//Просто номер
		Sensors[i].Name[2] = 0;
		Sensors[i].State = 0;										//Пока считается что все флаги сброшены
		Sensors[i].Adr = SENSOR_ADR_MASK-(i<<1);					//Адрес датчика по умолчанию от маски до 0
	}
	EeprmReadSensor();

	for(i=0;i<SENSOR_MAX;i++){
		Sensors[i].SleepPeriod = 0;														//Значения по умолчанию. Датчик пока не прислал ни одного значения
		Sensors[i].State = SensorFlgClr(Sensors[i]);									//Флаги состояния датчика сбросить, оставить только внутренние флаги
		SensorNoInBus(Sensors[i]);														//Датчик пока не обнаружен
	}
}

/************************************************************************/
/* Установка значения датчика. Вызывается при приеме полного пакета		*/
/*  приемником данных. 							                        */
/*  В параметре Adr значимыми являются биты 1,2,3						*/
/*  В параметре Stat значимыми являются биты 4,5,6,7					*/
/*  Возвращает 1 если датчик в режиме немедленного отображения			*/
/************************************************************************/
u08 SetSensor(u08 Adr, u08 Stat, u08 Value){
	u08 Ret = SENSOR_SHOW_NORMAL;
	for(u08 i=0; i<SENSOR_MAX; i++){							//Поиск описания датчика в списке датчиков
		if (Sensors[i].Adr == (Adr & SENSOR_ADR_MASK)){			//Описание для этого датчика найдено
			Sensors[i].State = (SensorFlgClr(Sensors[i]))		//Сохранить внутренние флаги
								| (Stat & SENSOR_STATE_MASK);	//Запомнить флаги самого датчика
			Sensors[i].Value = Value;
			Sensors[i].SleepPeriod = SENSOR_MAX_PERIOD;
			espSendSensor(i);									
			if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite) && (CurrentSensorShow == i)){ //Текущий сенсор находится в режиме ожидания, надо вывести сообщение на дисплей и пикнуть
				if (Refresh != NULL){
					Refresh();
					Ret = SENSOR_SHOW_TEST;
				}
			}
			break;
		}
	}
	return Ret;
}
