/*
 * Воспроизведение звука
 */ 

// TODO НЕ РЕАЛИЗОВАН разный режим выключения звука будильника - совсем выключение или только на время

#ifndef SOUND_H_
#define SOUND_H_

#include "pff.h"
#include "avrlibtypes.h"
#include "bits_macros.h"

#define ALARM_FILE_NAME			"alarm.wav"	//Имя файла для звука будильника
#define EACH_HOUR_FILE_NAME		"each.wav"	//Имя файла для звука каждый час
#define ONE_HOUR_FILE_NAME		"one.wav"	//Имя файла для звука боя. Звучит столько раз сколько в настоящее время часов

//Типы сигналов
#define SND_KEY_BEEP			0			//Звук кнопок
#define SND_ALARM_BEEP			1			//Звук будильника
#define SND_EACH_HOUR			2			//Звук курантов
#ifdef VOLUME_IS_DIGIT
	#define SND_TEST			3			//Тестовый звук, используется при настройке громкости
	#define SND_TEST_EACH		4			//Тест звука курантов
	#define	SND_TEST_ALARM		5			//Тест звука будильника
#endif
#define SND_SENSOR_TEST			10			//Звук при получении пакета данных от внешнего сенсора

//------- Величина буферов для звука, желательно кратно 512, т.к. Petit_FatFs наиболее быстро работает именно с такими кусками
#define BUF_LEN			512//256
#if (BUF_LEN != 512)
	#warning "The size of the buffer of memory not 512 bytes, are possible operation deceleration"
#endif

extern FATFS fs;							//FatFs object
extern unsigned char read_buf0[BUF_LEN], read_buf1[BUF_LEN];//Сами буфера выборки из файла, вынесен наружу т.к. используется в других модулях
extern volatile unsigned char	buf1_empty, buf0_empty;		//Состояние буферов

void SoundIni(void);						//Инициализация звуковой системы
void EachHourSettingSetOff(void);			//Выключить куранты
void EachHourSettingSwitch(void);			//Переключить разрешение на куранты
u08 EachHourSettingIsSet(void);				//Проверить флаг звучания сигнала каждый час. 0 - выключен
void KeyBeepSettingSetOff(void);			//Выключить звук кнопок
void KeyBeepSettingSwitch(void);			//Переключить разрешение звука кнопок
u08 KeyBeepSettingIsSet(void);				//Проверить флаг звучания кнопок. 0 - звук кнопок выключен
void SoundOn(u08 SoundType);				//Включить сигнал указанного типа
void SoundOff(void);						//Выключить сигнал
u08 AlarmBeepIsSound(void);					//Сигнал будильника в текущий момент звучит? 0 - нет, 1 - да
u08 SoundIsBusy(void);						//Звуковая система заняла fatFs 0 - свободно, 1 - занято

#endif /* SOUND_H_ */
