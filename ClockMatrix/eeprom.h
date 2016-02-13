/*
 * Спасение настроек часов в EEPROM
 * Используется весь массив eeprom для равномерного расходования ресурса flash-ячеек. Не знаю на сколько это вообще нужно, но прикольно.
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include "Clock.h"
#include "Alarm.h"

#define EEP_READY		1							//Память корректна
#define EEP_BAD			0							//в eeprom нет коректных данных

void EepromInit(void);								//Инициализация указателей и состояния EEPROM
u08 EeprmReadAlarm(struct sAlarm* Alrm);			//чтение настроек будильников из EEPROM
u08 EeprmReadSensor(void);							//Чтение настроек сенсоров
u08 EeprmReadIRCode(u08 Idx);						//Чтение кодов пульта ДУ
u08 EeprmReadTune(void);							//Чтение настроек часов
void EeprmStartWrite(void);							//Начать запись состояния в EEPROM. Записывается все разом - будильники и настройки часов

#endif /* EEPROM_H_ */