/*
 * Регулировка громкости
 */ 

#include <stddef.h>
#include "EERTOS.h"
#include "Clock.h"
#include "VolumeHAL.h"
#include "Volume.h"

struct sVolume VolumeClock[VOL_TYPE_COUNT];				//Настройки уровней громкости для разных режимов
u08 IncreaseMaxVolume = 0,								//Уровень до которого надо поднимать громкость при постепенном повышении
	CurrenVolume = 0;									//Текущий уровень громкости

/************************************************************************/
/* Инициализация регулятора громкости                                   */
/************************************************************************/
void VolumeIni(void){
	VolumeIntfIni();									//Настроить порт и регулятор
	VolumeClock[vtAlarm].LevelVol = vlIncreaseMax;		//Настройки громкости по умолчанию
	VolumeClock[vtAlarm].Volume = VOLUME_LEVEL_MAX;
	VolumeClock[vtButton].Volume = 3;
	VolumeClock[vtButton].LevelVol = vlConst;
	VolumeClock[vtEachHour].Volume = VOLUME_LEVEL_MAX>>1;
	VolumeClock[vtSensorTest].Volume = VOLUME_LEVEL_MAX;
	VolumeClock[vtSensorTest].LevelVol = vlConst;
}

void VolumeAddLevel(enum tVolumeType Value){
	if (VolumeClock[Value].Volume == VOLUME_LEVEL_MAX)	//Максимальная громкость?
		VolumeClock[Value].Volume = 0;
	else
		VolumeClock[Value].Volume++;
	VolumeSet(VolumeClock[Value].Volume);
	VolumeOnHard;
	if (Refresh != NULL)
		Refresh();
}
	
/************************************************************************/
/* Регулировка громкости нажатия клавиш                                 */
/************************************************************************/
void VolumeKeyBeepAdd(void){
	VolumeAddLevel(vtButton);
}

/************************************************************************/
/* Регулировка громкости ежечастного боя                                */
/************************************************************************/
void VolumeEachHourAdd(void){
	VolumeAddLevel(vtEachHour);
}
/************************************************************************/
/* Регулировка громкости будильника		                                */
/************************************************************************/
void VolumeAlarmAdd(void){
	VolumeAddLevel(vtAlarm);
}

/************************************************************************/
/* Переключение типа регулировки громкости для будильника               */
/************************************************************************/
void VolumeTypeTuneAlarm(void){
	VolumeClock[vtAlarm].LevelVol++;
	if (VolumeClock[vtAlarm].LevelVol == vlLast)//Достигнут конец перечисления?
		VolumeClock[vtAlarm].LevelVol = 0;
	if (Refresh != NULL)
		Refresh();
}

/************************************************************************/
/* Проверяет тип регулировки уровня громкости будильника.               */
/* Возвращает 0 тип регулировки не требует указания уровня или			*/
/* 1 если требуется указание уровня громкости							*/
/************************************************************************/
u08 VolumeAlrmTypeIsNeedLevel(void){
	return (VolumeClock[vtAlarm].LevelVol == vlIncreaseMax)?0:1;
}

/************************************************************************/
/* Постепенный подъем громкости до заданного уровня                     */
/************************************************************************/
void VolumeIncrease(void){
	if (CurrenVolume <= IncreaseMaxVolume){				//Необходимая громкость еще не достигнута, продолжаем
		CurrenVolume++;
		VolumeSet(CurrenVolume);
		SetTimerTask(VolumeIncrease, VOL_CHANGE_LVL_TIME);
	}
	else{												//Необходимая громкость достигнута, останавливаемся
		CurrenVolume = 0;								//Готов к следующьему циклу
		IncreaseMaxVolume = 0;
	}
}

/************************************************************************/
/* Запустить регулирование уровня громкости для данного типа            */
/************************************************************************/
void VolumeAdjustStart(enum tVolumeType Value){
	VolumeSet(0);
	VolumeOnHard;
	if (VolumeClock[Value].LevelVol != vlConst){			//Нарастающий звук, запускается задача нарастания уровня громкости
		if (VolumeClock[Value].LevelVol == vlIncreas)
			IncreaseMaxVolume = VolumeClock[Value].Volume;
		else
			IncreaseMaxVolume = VOLUME_LEVEL_MAX;
		VolumeIncrease();									//Начать постепенный подъем уровня громкости
	}
	else
		VolumeSet(VolumeClock[Value].Volume);
}

/************************************************************************/
/* Выключение-включение звука                                           */
/************************************************************************/
void VolumeOff(void){
	CurrenVolume = 0;										//Текущий уровень громкости
	VolumeOffHard;	
}

void VolumeOn(void){
	VolumeOnHard;
}
