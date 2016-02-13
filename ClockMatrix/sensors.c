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

// Формат строки описания датчика на sd карте в файле sensor.txt: 1=1эт, где 1 - номер датчика, = знак равно, за которым идет не более трех букв описания датчика
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
	//TODO: Добавить вызов этой функции при появлении SD кар/точки в слоту
//	FATFS fs;
//	WORD rb;														//Прочитанных байт
//	u08 CountSens = 0;
	u08 i = 0;
	
	for(;i<SENSOR_MAX;i++){
		Sensors[i].Name[0] = 'д';
		Sensors[i].Name[1] = 0x30 | i;								//Просто номер
		Sensors[i].Name[2] = 0;
		Sensors[i].State = 0;										//Пока считается что все флаги сброшены
		Sensors[i].Adr = SENSOR_ADR_MASK-(i<<1);					//Адрес датчика по умолчанию от маски до 0
	}
	EeprmReadSensor();
/*	
	if (!EeprmReadSensor()){										//Не удалось прочитать настройки датчиков из eeprom, ищем на sd-карте
		if (pf_disk_is_mount() != FR_OK)							//Карта еще не определялась, пробуем ее инициализировать.
			if (pf_mount(&fs) == FR_OK)
				if (pf_open("sensor.txt") == FR_OK)
					if (pf_lseek (0) == FR_OK)
						if (pf_read(&read_buf0, BUF_LEN, &rb) == FR_OK){
							while(i <= rb){
								if	( (read_buf0[i] >= 0x30) && (read_buf0[i]<=0x39)	//Первый символ цифра, 
									  && (read_buf0[i+1] == '=')						//а второй знак равно
									){													//Разбор строки
									Sensors[CountSens].Adr = ((read_buf0[i] & 0xf)<<1);	//Номер датчика в списке датчиков
									for(u08 j = 0;j < SENSOR_LEN_NAME; j++)				//Подготовить строку для имени датчика
										Sensors[CountSens].Name[j] = 0;
									for(u08 j = 0;j < SENSOR_LEN_NAME; j++){			//Запомнить имя сенсора
										if ((read_buf0[(i+2)+j] == 0xd) || (read_buf0[(i+2)+j] == 0xa) || (read_buf0[(i+2)+j] == 0) || ((i+2+j)>rb) )
											break;										//Конец строки или файла
										Sensors[CountSens].Name[j] = read_buf0[(i+2)+j];
									}
									SensorShowOn(Sensors[CountSens]);					//Включить вывод датчика
									CountSens++;										//Следующее описание датчика
									if (CountSens > SENSOR_MAX) break;					//Больше нет описаний датчиков
								}
								while((read_buf0[i] != 0xd) && (i<=rb)) i++;			//Следующая строка
								if (read_buf0[i] == 0xa) i++;							//Пропустить служебный символ если он есть
								if (i>=rb) break;										//достигут конец буфера
							}
						}
	}
	*/
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
/*  В параметре Stat значимыми являются биты 4,5,6,7					 */
/************************************************************************/
void SetSensor(u08 Adr, u08 Stat, u08 Value){
	for(u08 i=0; i<SENSOR_MAX; i++){							//Поиск описания датчика в списке датчиков
		if (Sensors[i].Adr == (Adr & SENSOR_ADR_MASK)){			//Описание для этого датчика найдено
			Sensors[i].State = (SensorFlgClr(Sensors[i]))		//Сохранить внутренние флаги
								| (Stat & SENSOR_STATE_MASK);	//Запомнить флаги самого датчика
			Sensors[i].Value = Value;
			Sensors[i].SleepPeriod = SENSOR_MAX_PERIOD;
			espSendSensor(i);
			if (SensorTestModeIsOn(Sensors[i])){				//Текущий сенсор находится в режиме тест, надо вывести сообщение на дисплей и пикнуть
				SetClockStatus(csTune, ssSensorTest);
				SoundOn(SND_SENSOR_TEST);
			}
			break;
		}
	}
}
