/*
 * Регулировка громкости
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

#include "avrlibtypes.h"

#define VOL_TYPE_COUNT		4				//Количество поддерживаемых типов регулирования громкости
//Типы настройки регулятора звука
enum tVolumeType{
	vtButton = 0,							//Громкость звука кнопок
	vtEachHour = 1,							// -/-       -/-  каждый час
	vtAlarm = 2,							// -/-       -/-  будильника
	vtSensorTest = 3						// -/-       -/-  получения тестового сигнала от передатчика внешних датчиков
	};

//Типы регулирования громкости
enum tVolumeLevel{
	vlConst = 1,							//Постоянный настраиваемый уровень громкости
	vlIncreas = 2,							//Нарастающий уровень до настраиваемого уровня
	vlIncreaseMax = 3,						//Нарастающий уровень до максимального уровня
	vlLast = 4,								//Последний элемент, используется только как признак конца перечисления
	};

#define VOLUME_LEVEL_MAX	64				//Максимальное значение громкости выше которого регулирование громкости не заметно
#define VOL_CHANGE_LVL_TIME	250				//Время между шагами при плавном увеличении громкости, мс

//Значение громкости для определенного типа
struct sVolume{
	enum tVolumeLevel LevelVol;				//Тип регулирования
	u08 Volume;								//Значение громкости для этого типа
	};

extern struct sVolume VolumeClock[VOL_TYPE_COUNT];

void VolumeIni(void);						//Инициализация регулятора громкости
void VolumeKeyBeepAdd(void);				//Увеличить громкость звука для кнопок
void VolumeEachHourAdd(void);				//Регулировка громкости ежечастного боя
void VolumeAlarmAdd(void);					//Регулировка громкости будильника
void VolumeTypeTuneAlarm(void);				//Переключение типа регулировки громкости для будильника
void VolumeOff(void);						//Отключение звука
void VolumeOn(void);						//Включение звука
u08 VolumeAlrmTypeIsNeedLevel(void);		//Проверяет тип регулировки уровня громкости будильника

void VolumeAdjustStart(enum tVolumeType Value);//Запустить регулирование уровня громкости для данного типа

#endif /* INCFILE1_H_ */