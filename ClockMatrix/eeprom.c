/*
 * Спасение настроек часов в EEPROM
 * ВНИМАНИЕ! Из-за возможности работы без регулятора громкости сохранение настройки часов должно производится последним!
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>
#include "eeprom.h"
#include "bits_macros.h"
#include "EERTOS.h"
#include "i2c.h"
#include "Sound.h"
#include "Display.h"
#include "sensors.h"
#include "IRrecive.h"
#ifdef VOLUME_IS_DIGIT
#include "Volume.h"
#endif

typedef u08 (*EP_EVNT)(u08* Data);											//тип указателя на функцию обработки этапа записи. Записывает в Data байт который необходимо записать и возвращает 0 если Data действительно и 1 если содержимое Data некоректно
#define EP_NORMAL				0											//Нормальная работа функции EP_EVNT в соответствии с внутренним счетчиком шагов
#define EP_END					1											//Работа автомата завершена, содержимое Data некорректно и не требует записи
volatile EP_EVNT				epEndEvent;									//функция вызываемая после записи одного байта

#define epWriteStop()			ClearBit(EECR, EERIE);						//Запретить прерывания от EEPROM, а следовательно и закончить запись
#define epWriteStart()			SetBit(EECR, EERIE);						//Разрешить прерывание от блока записи - старт записи

#define epAlarmSize				(sizeof(struct sAlarm))						//Размер области eeprom для одного будильника
#define epSensSize				(sizeof(SensorFromIdx(0).Name)+sizeof(SensorFromIdx(0).Adr)+sizeof(SensorFromIdx(0).State)) //Размер eeprom для одного сенсора
#define EEPROM_SENSOR_ADR		0											//Номер байта адреса сенсора в пакете
#define EEPROM_SENSOR_STATE		1											//Адрес байта состояния

u16 epAdr;																	//Адрес начала пакета. Если равно EEPROM_NULL то адрес не корректен
volatile u16 epAdrWrite;													//Текущий адрес записи в eeprom

#define EEPROM_SIGNATURE		0xaa										//Настройки в EEPROM действительны
#define EEPROM_NO_SIGNATURE		0x55										//Настройки в EEPROM не действительна, необходимо проверить следующий пакет

#define EEPROM_BASE				0											//Базовый адрес с которого начинается хранение в eeprom
#define EEPROM_NULL				(E2END+1)									//Признак некорректности адреса eeprom

//Байты настройки часов
#define EEPROM_TUNE_COUNT		4											//Количество байт для хранения настроек. Первый байт это номер шрифта цифр. Второй разные флаги. Третий и четвертый скорость бегущей строки
#define EEPROM_TUNE_FONT		0											//Байт 0 для номера шрифта
#define EEPROM_TUNE_FLAG		1											//Различные флаги
#define EEPROM_TUNE_HZ_SPEED1	2											//Скорость бегущей строки - старший байт
#define EEPROM_TUNE_HZ_SPEED2	3											//Скорость бегущей строки - младший 

//Биты хранения байта флагов во втором байте хранения настроек
#define EEPROM_TUNE_EACH_HOUR	7											//Номер бита включения курантов
#define EEPROM_TUNE_KEY_BEEP	6											//Писк кнопок
//5 свободные биты фагов
//4
//3
//2
//1
//0

//Коды пульта ДУ
#define epIrCodeSize			(IR_NUM_COMMAND*IR_NUM_BYTS)				//Размер массива для хранения кодов команд ИК пульта
#if (epIrCodeSize > 256)
	#warning "epIrCodeSize is big!"
#endif

//Настройка регулятора громкости
#ifdef VOLUME_IS_DIGIT
#define EEPROM_TUNE_EACH_VOL	0											//Байт уровня громкости курантов
#define EEPROM_TUNE_KEY_VOL		1											//Громкость кнопок
#define EEPROM_TUNE_ALARM_VOL	2											//Громкость будильника
#define EEPROM_TUNE_ARARM_TYPE_VOL	3										//Режим регулировки громкости будильника
#define EEPROM_TUNE_ADD_BYTE	(EEPROM_TUNE_ARARM_TYPE_VOL+1)				//Количество дополнительных байт для хранения уровня и режима громкости
	#if ((EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_ALARM_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_KEY_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_KEY_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_EACH_VOL))
		#error "EEPROM_TUNE_ARARM_TYPE_VOL to be the largest number of!"
	#endif
#else
#define EEPROM_TUNE_ADD_BYTE	0											//Без цифрового регулятора не используется
#endif

//ВНИМАНИЕ! Из-за возможности работы без регулятора громкости сохранение настройки часов должно производится последним!
#define epPacketSize			((epAlarmSize*ALARM_MAX)+(epSensSize*SENSOR_MAX)+EEPROM_TUNE_COUNT+epIrCodeSize+EEPROM_TUNE_ADD_BYTE+1)	//Размер пакета. число 1 это место для сигнатуры актуальности данных
#define epAlarmOffset			(epAdr+1)									//Смещение для настроек будильника от начала пакета
#define epSensorOffset			(epAlarmOffset+(epAlarmSize*ALARM_MAX))		//Для сенcоров
#define epIrCodeOffset			(epSensorOffset+(epSensSize*SENSOR_MAX))	//Для кодов пульта ДУ
#define epTuneOffset			(epIrCodeOffset+epIrCodeSize)				//Для настроек часов и возможно громкости

//-------- Самые низкоуровневые вызовы
#define eepromWriteBusy()		BitIsSet(EECR, EEWE)						//Память занята записью
#define eepromWriteIsFree()		BitIsClear(EECR, EEWE)						//Память не занята процессом записи

volatile u08 epStepWr, epCountWr;											//Счетчики шагов и перебора переменных внутри шагов в функциях записи. 
																			//Счетчик шагов показывает сколько раз была вызвана функция из прерывания окончания записи

/************************************************************************/
/* Чтение байта из eeprom									            */
/************************************************************************/
u08 EeprmRead(volatile const u16 Adr){
	while(eepromWriteBusy());												//Ожидание завершения предыдущей операции с EEPROM
	EEAR = Adr;
	SetBit(EECR, EERE);
	return EEDR;
}

/************************************************************************/
/* Инициализация памяти eeprom - проверяется наличие корректности       */
/************************************************************************/
void EepromInit(void){
	u08	Sig;

	epAdr = EEPROM_BASE;
	do{																		//Поиск актуальных данных списке пакетов
		Sig = EeprmRead(epAdr);
		epAdr += epPacketSize;												//Следующий адрес пакета
	} while ((Sig == EEPROM_NO_SIGNATURE) && (epAdr < (E2END-epPacketSize)));
	if (Sig == EEPROM_SIGNATURE)											//Если данные актуальны то адрес eeprom указывает на них, в противном случае NULL
		epAdr -= epPacketSize;												//Отматываем назад лишние байты в адресе
	else
		epAdr = EEPROM_NULL;
	epWriteFree();															//Обмен завершен транзакция завершена
}

/************************************************************************/
/* Проверка готовности пакета и памяти EEPROM.							*/
/* Если память занята операцией записи то ждет некоторое время			*/
/* готовность															*/
/* Возвращает EEP_READY если можно проводить операции с памятью,		*/
/*	EEP_BAD - нет														*/
/************************************************************************/
u08 EepromReady(void){
	u08 White = 0xff;

	if (epAdr != EEPROM_NULL){												//Память корректна
		while (epWriteIsBusy()){											//Процесс работы с eeprom еще занят
			White--;
			if (White == 0)
				return EEP_BAD;												//Слишком долго ждали
		}
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* чтение настроек будильника в область памяти Alrm						*/
/* Возвращает 1 если прочитано удачно и 0 если прочитать не удалось		*/
/************************************************************************/
u08 EeprmReadAlarm(struct sAlarm* Alrm){
	
	u08 Buf[epAlarmSize];

	if (EepromReady()){
		for(u08 Count=0; Count<epAlarmSize; Count++)						//Прочитать будильник из eeprom в буфер
			Buf[Count] = EeprmRead((epAlarmOffset+(epAlarmSize*(Alrm->Id)))+Count);
		memcpy(Alrm, &Buf, epAlarmSize);									//заполнить структуру будильника из буфера
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* чтение настроек сенсоров												*/
/* Возвращает 1 если прочитано удачно и 0 если прочитать не удалось		*/
/************************************************************************/
u08 EeprmReadSensor(void){
	
	u08 Buf[epSensSize];

	if (EepromReady() == 0)													//Память не готова к чтению
		return EEP_BAD;
	for(u08 SensCount = 0; SensCount<SENSOR_MAX; SensCount++){
		for(u08 Count=0; Count<epSensSize; Count++)							//Прочитать сенсор из eeprom в буфер
			Buf[Count] = EeprmRead(epSensorOffset+epSensSize*SensCount+Count);
		SensorFromIdx(SensCount).Adr = Buf[EEPROM_SENSOR_ADR];				//Адрес сенсора на шине
		SensorFromIdx(SensCount).State = Buf[EEPROM_SENSOR_STATE];			//Состояние сенсора
		for(u08 i=0;i<SENSOR_LEN_NAME;i++)									//Имя сенсора
			SensorFromIdx(SensCount).Name[i] = Buf[EEPROM_SENSOR_STATE+1+i];
	}
	return EEP_READY;
}

/************************************************************************/
/* Чтение настроек часов из EEPROM.										*/
/* Возвращает 1 если удалось прочитать, 0 - если не удалось             */
/************************************************************************/
u08 EeprmReadTune(void){
	u08 Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ADD_BYTE];
	
	if (EepromReady() !=0){
		for(u08 Count=0;Count<(EEPROM_TUNE_COUNT+EEPROM_TUNE_ADD_BYTE);Count++)	//читаем байты настройки
			Buf[Count] = EeprmRead(epTuneOffset+Count);
		FontIdSet(Buf[EEPROM_TUNE_FONT]);									//Шрифт цифр
		if (FontIdGet() > FONT_COUNT_MAX)									//Вдруг не тот номер шрифта прочитается
			FontIdSet(0);
		//u08 i = FontIdGet();
		//espUartTx(ClkIsWrite(CLK_FONT), &i, 1);
		EachHourSettingSetOff();											//Куранты
		if BitIsSet(Buf[EEPROM_TUNE_FLAG], EEPROM_TUNE_EACH_HOUR)
			EachHourSettingSwitch();										//Включить если нужно
		KeyBeepSettingSetOff();												//Писк кнопок
		if BitIsSet(Buf[EEPROM_TUNE_FLAG], EEPROM_TUNE_KEY_BEEP)
			KeyBeepSettingSwitch();											//Включить если нужно
		HorizontalSpeed = ((u16)Buf[EEPROM_TUNE_HZ_SPEED1]<<8)+((u16)Buf[EEPROM_TUNE_HZ_SPEED2]);//Скорость горизонтальной прокрутки
		if ((HorizontalSpeed >HORZ_SCROLL_MIN) || (HorizontalSpeed<HORZ_SCROLL_MAX)){	//Прочитанная скорость некорректна
			HorizontalSpeed = HORZ_SCROLL_MAX+HORZ_SCROOL_STEP*HORZ_SCROLL_STEPS/2;	//ПО умолчанию средняя скорость прокрутки
		}
#ifdef VOLUME_IS_DIGIT
		VolumeClock[vtEachHour].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_EACH_VOL];//Байт уровня громкости курантов
		VolumeClock[vtButton].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_KEY_VOL];//Громкость кнопок
		VolumeClock[vtAlarm].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ALARM_VOL];//Громкость будильника
		VolumeClock[vtAlarm].LevelVol = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ARARM_TYPE_VOL];//Режим регулировки громкости будильника
#endif
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* Чтение настроек кодов пульта ДУ.										*/
/* Idx - индекс кода в массиве IRcmdList                                */
/************************************************************************/
u08 EeprmReadIRCode(const u08 Idx){

	if (EepromReady() && (Idx<IR_NUM_COMMAND)){
		IRcmdList[Idx] = 0;
		for (u08 Count=0; Count < IR_NUM_BYTS; Count++){
			IRcmdList[Idx] |= ((u32)EeprmRead(epIrCodeOffset+Count+(Idx*IR_NUM_BYTS))<<(u32)MULTIx8(Count));
		}
		return EEP_READY;
	}
	return EEP_BAD;
}

//------------------------ ЗАПИСЬ ----------------------------------

/************************************************************************/
/* Возвращает адрес в eeprom для следующего пакета                      */
/************************************************************************/
u16 epAdrNextPacket(void){
	if ((epAdr == EEPROM_NULL) || ((epAdr+epPacketSize) > E2END))		//Адрес пакета еще не определен или пакет уходит за границу адресов памяти eeprom, адрес с начала памяти eeprom
		return EEPROM_BASE;
	else
		return epAdr+epPacketSize;										//Текущий адрес сдвигаем на размер пакета
}

/************************************************************************/
/* Отметка пакета об актуальности                                       */
/************************************************************************/
u08 epPacketNewWrite(u08* Value){
	
	switch (epStepWr){
		case 0:															//Отмечаем актуальность нового пакета
			epAdrWrite = epAdrNextPacket();								//Возвращаемся к началу записи нового пакета
			*Value = EEPROM_SIGNATURE;
			break;
		case 1:
			if (epAdr != EEPROM_NULL){									//Был предыдущий пакет, помечаем его как неактуальный
				epAdrWrite = epAdr;
				*Value = EEPROM_NO_SIGNATURE;
				break;													//Прерываем выполнение switch что бы  записать признак неактуальности
			}
		case 2:															//Операция записи завершена
			epAdr = epAdrNextPacket();									//Адрес пакета сдвигается на размер пакета
			return EP_END;
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;
}

/************************************************************************/
/* Запись настроек громкости                                            */
/************************************************************************/
#ifdef VOLUME_IS_DIGIT
u08 epVolumeWrite(u08* Value){
	switch (epStepWr){
		case EEPROM_TUNE_EACH_VOL:
			*Value = VolumeClock[vtEachHour].Volume;					//Байт уровня громкости курантов
			break;
		case EEPROM_TUNE_KEY_VOL:
			*Value = VolumeClock[vtButton].Volume;						//Громкость кнопок
			break;
		case EEPROM_TUNE_ALARM_VOL:
			*Value = VolumeClock[vtAlarm].Volume;						//Громкость будильника
			break;
		case EEPROM_TUNE_ARARM_TYPE_VOL:
			*Value = VolumeClock[vtAlarm].LevelVol;						//Режим регулировки громкости будильника
			break;
		case (EEPROM_TUNE_ARARM_TYPE_VOL+1):							//Запись настроек громкости завершена, записывается метка актуальности
			epEndEvent = epPacketNewWrite;
			epStepWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;											//Запись в eeprom еще не завершена
			break;
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;													//Запись в eeprom еще не завершена
}
#endif

/************************************************************************/
/* Запись настроек часов                                                */
/* Эта функция должна быть последней в цепочке записи т.к. после нее    */
/* возможно будет запись громкости									    */
/************************************************************************/
u08 epTuneWrite(u08* Value){
	u08 i = 0;
	
	switch (epStepWr){
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_FONT)):	//0-й байт - Номер шрифта. define EEPROM_TUNE_COUNT введен для того что бы видеть где нужно изменить case если будет изменятся состав настроек
			*Value = FontIdGet();
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_FLAG)):	//1-й байт - Разные настройки
			if (EachHourSettingIsSet())									//Куранты
				SetBit(i, EEPROM_TUNE_EACH_HOUR);
			if (KeyBeepSettingIsSet())									//Звук нажатия кнопок
				SetBit(i, EEPROM_TUNE_KEY_BEEP);
				*Value = i;
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_HZ_SPEED1)):	//2-й байт - Скорость бегущей строки, старший байт
			*Value = (u08)(HorizontalSpeed>>8);
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_HZ_SPEED2)):	//3-й байт - Скорость бегущей строки, младший байт
			*Value = (u08)(HorizontalSpeed);
			break;
		case EEPROM_TUNE_COUNT:											//Настройки записаны.
			#ifdef VOLUME_IS_DIGIT
				epEndEvent = epVolumeWrite;								//Следующая запись настроек громкости
			#else
				epEndEvent = epPacketNewWrite;							//или отметить актуальность пакета если микросхемы регулятора громкости нет
			#endif
			epStepWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;
			break;		
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;													//Запись в eeprom еще не завершена
}

/************************************************************************/
/* Запись кодов пульта ДУ                                               */
/************************************************************************/
u08 epIRCodeWrite(u08* Value){

	if (epCountWr == IR_NUM_BYTS){										//Был записан последний байт кодов
		epStepWr++;
		epCountWr = 0;
		if (epStepWr == IR_NUM_COMMAND) {								//Был записан последний код
			epStepWr = 0;
			epEndEvent = epTuneWrite;									//Следующая запись настроек часов
			epEndEvent(Value);
			return EP_NORMAL;
		}
	}
	*Value = (u08)(IRcmdList[epStepWr]>>MULTIx8(epCountWr) & 0xff);		//Подготовка очередного байта очередного кода
	epCountWr++;
	return EP_NORMAL;
}

/************************************************************************/
/* Запись настроек сенсоров												*/
/************************************************************************/
u08 epSensorsWrite(u08* Value){

	static u08 SensorNameCount;
	
	if (epStepWr == EEPROM_SENSOR_ADR)									//Запись адреса сенсора
		*Value = SensorFromIdx(epCountWr).Adr;
	else if (epStepWr == EEPROM_SENSOR_STATE){							//Запись состояния сенсора
		*Value = SensorFromIdx(epCountWr).State;
		SensorNameCount = 0;
	}
	else if ((epStepWr >EEPROM_SENSOR_STATE) && ((epStepWr-(EEPROM_SENSOR_STATE+1)) < SENSOR_LEN_NAME)){//Имя сенсора
		*Value = SensorFromIdx(epCountWr).Name[SensorNameCount];
		SensorNameCount++;
	}
	else{																//Один сенсор записан
		epCountWr++;													//Следующий сенсор
		if (epCountWr == SENSOR_MAX){									//Все сенсоры записаны
			epEndEvent = epIRCodeWrite;									//Следующая запись кодов пульта ДУ
			epStepWr = 0;
			epCountWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;
		}
		else{															//Сенсоры еще не все записаны, продолжаем запись
			epStepWr = 0;												//Следующий сенсор записывается
			*Value = SensorFromIdx(epCountWr).Adr;
		}
	}
	epStepWr++;
	return EP_NORMAL;													//Запись в eeprom еще не завершена
}

/************************************************************************/
/* Запись будильников                                                   */
/************************************************************************/
u08 epAlarmWrite(u08* Value){
	
	u08 Buf[epAlarmSize];
	
	if (epStepWr == epAlarmSize){										//Один будильник записан
		epCountWr++;													//Следующий будильник
		if (epCountWr == ALARM_MAX){									//Все будильники записаны, переходим к следующему байту записи
			epEndEvent = epSensorsWrite;									
			epStepWr = 0;
			epCountWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;											//Запись в eeprom еще не завершена
		}
		epStepWr = 0;
	}
	memcpy(&Buf, ElementAlarm(epCountWr), epAlarmSize);					//Прочитать будильник в буфер
	*Value = Buf[epStepWr];
	epStepWr++;															//Следующий байт
	return EP_NORMAL;													//Запись в eeprom еще не завершена
}

/************************************************************************/
/* Старт записи установок часов в EEPROM. В случае если запись еще		*/
/* идет задача вставляет себя в очередь снова                           */
/************************************************************************/
void EeprmStartWrite(void){
	if (epWriteIsBusy()) 												//Запись еще идет
		SetTask(EeprmStartWrite);										//Ожидаем завершения
	else{																//Можно запускать запись в EEPROM
		epWriteBusy();													//Занять автомат
		epAdrWrite = epAdrNextPacket();
		epStepWr = 0;
		epCountWr = 0;
		epEndEvent = epAlarmWrite;										//Сначала записывается состояние будильников
		epAdrWrite++;													//Оставляем один байт для метки актуальности пакета
		epWriteStart();													//Старт записи
	}
}

/************************************************************************/
/* Запись байта по адресу epAdrWrite                                    */
/************************************************************************/
inline void WrieByteEeprom(u08 Value){
	while(eepromWriteBusy());											//Ожидание завершения предыдущей операции с EEPROM
	EEAR = epAdrWrite;
	SetBit(EECR, EERE);
	if (EEDR != Value){													//Байты не совпадают, надо переписать
		EEDR = Value;
		SetBit(EECR, EEMWE);											//Разрешить запись
		SetBit(EECR, EEWE);												//Пишется
	}
}

/************************************************************************/
/* Запись очередного байта закончена берется следующий байт             */
/************************************************************************/
ISR(EE_RDY_vect){
	u08 Data;
	
	if (epEndEvent(&Data) != EP_NORMAL){								//Прочитать очередной байт для записи в EEPROM
		epWriteStop();													//Работа автомата закончена, запретить прерывания от EEPROM
		epWriteFree();													//Освободить автомат
	}
	else{																//Работа автомата еще не закончена, надо записать полученный байт
		WrieByteEeprom(Data);
		epAdrWrite++;													//Следующий адрес
	}
}
