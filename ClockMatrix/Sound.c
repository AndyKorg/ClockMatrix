/************************************************************************/
/* Вывод звука. МК читает и воспроизводит файл из SD карточки			*/
/* Используются таймеры:												*/
/* Таймер 0 - определяет скважиность ШИМ								*/
/* Таймер 1 - определяет битрейт										*/
/* ver 1.4																*/
/************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "EERTOS.h"
#include "Clock.h"
#include "i2c.h"												//Значение текущего времени для курантов
#include "Sound.h"
#ifdef VOLUME_IS_DIGIT
	#include "Volume.h"
#endif

#ifndef F_CPU
#warning "F_CPU not define"
#endif

//------- Воспроизведение звука
#define STOP_TMR_BITRT	ClearBit(TIMSK, OCIE1A)					//Стоп таймер воспроизведения
#define START_TMR_BITRT	SetBit(TIMSK, OCIE1A)					//Старт таймер воспоизведения
#define SOUND_OFF		TCCR0 &= ~( Bit(COM01) | Bit(COM00) )	//Выключить звук - отмена дрыгания ножкой OC0
#define SOUND_ON		TCCR0 |= ( Bit(COM01) | Bit(COM00) )	//Включить звук

//------- Описание буферов памяти
#define BUF_EMPTY		1										//Буфер пуст
#define BUF_FULL		0
#define BUF0_READ		1										//читать из буфера 0
#define BUF1_READ		0										//читать из буфера 1

//Хотя эти переменные изменяются в прерывании, они не объявлены volatile, т.к. по логике работы они не изменяются одновременно в прерывании и в основном потоке
unsigned int PosInBuf;											//Текущая позиция в буфере
unsigned char reading0buf;										//Идет чтение из буфера 0, в противном случае из буфера 1
unsigned char read_buf0[BUF_LEN], read_buf1[BUF_LEN];			//Сами буфера выборки
volatile unsigned char	buf1_empty = BUF_EMPTY,					
						buf0_empty = BUF_EMPTY;					//Состояние буферов

FATFS fs;														//FatFs object
						
unsigned long int
	DataChunkSize,												//Количество частей звукового файла
	CurrentChunk;												//Текущая проигрываемая часть

u08 StrikingClockCount;											//Количество звуков боя часов
u08 SoundFlag = 0;												//Состояние сигнала (на кнопки, каждый час и т.п.)

//Флаги SoundFlag. Флаги текущего состояния звуковой системы помечены _WORK, флаги настройки _SETING
//Настройки звуковой системы
#define EACH_HOUR_BEEP_SETING	0											//Бой часов (куранты) каждый час
#define EachHourSettingOn		SetBit(SoundFlag, EACH_HOUR_BEEP_SETING)
#define EachHourSettingOff		ClearBit(SoundFlag, EACH_HOUR_BEEP_SETING)
#define EachHourSettingIsOn		BitIsSet(SoundFlag, EACH_HOUR_BEEP_SETING)

#define KEY_BEEP_SETING			1											//Звук на кнопках
#define KeyBeepSettingOn		SetBit(SoundFlag, KEY_BEEP_SETING)
#define KeyBeepSettingOff		ClearBit(SoundFlag, KEY_BEEP_SETING)
#define	KeyBeepSettingIsOn		BitIsSet(SoundFlag, KEY_BEEP_SETING)

//Текущее состояние
#define EACH_HOUR_BEEP_WORK		3											//Звук каждый час звучит
#define EachHourBegin			SetBit(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourEnd				ClearBit(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourIsWork			BitIsSet(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourIsNoWork		BitIsClear(SoundFlag, EACH_HOUR_BEEP_WORK)

#define KEY_BEEP_WORK			4											//Звук кнопки звучит
#define KeyBeepBegin			SetBit(SoundFlag, KEY_BEEP_WORK)
#define KeyBeepEnd				ClearBit(SoundFlag, KEY_BEEP_WORK)
#define KeyBeepIsWork			BitIsSet(SoundFlag, KEY_BEEP_WORK)

#define	ALARM_BEEP_WORK			5											//Сигнал будильника звучит
#define AlarmBeepBegin			SetBit(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepEnd			ClearBit(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepIsWork			BitIsSet(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepIsNoWork		BitIsClear(SoundFlag, ALARM_BEEP_WORK)

//------- Звук без файла 
#define BEEP_BITRATE		22050								//Битрейт для звука кнопок
#define BEEP_READ_PERIOD	5									//Период чтения данных в буфер для звука кнопок
#define BEEP_KEY_DELAY		25									//Длительность звучания для нажатой кнопки
#define BEEP_ON_DELAY		500									//Длительность включенного период писка для звука каждый час или будильника
#define BEEP_OFF_DELAY		500									//Длительность выключенного периода писка для звука каждый час или будильника
//TODO: Хреновый синус получается, что-то надо подкрутить или вообще переделать на воспроизведение с SD-card
#define BEEP_SIN_LEN		256									//Количество отсчетов в таблице синуса sin_table
const u08 PROGMEM sin_table[] =									//Таблица синуса для звука кнопок. 256 отсчетов
{
	0x7F, 0x7C, 0x79, 0x76, 0x73, 0x6F, 0x6C, 0x69, 0x66, 0x63, 0x60, 0x5D, 0x5A, 0x57, 0x54, 0x51,
	0x4E, 0x4C, 0x49, 0x46, 0x43, 0x40, 0x3E, 0x3B, 0x38, 0x36, 0x33, 0x31, 0x2E, 0x2C, 0x2A, 0x27,
	0x25, 0x23, 0x21, 0x1F, 0x1D, 0x1B, 0x19, 0x17, 0x15, 0x14, 0x12, 0x10, 0x0F, 0x0E, 0x0C, 0x0B,
	0x0A, 0x09, 0x07, 0x06, 0x05, 0x05, 0x04, 0x03, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x09,
	0x0A, 0x0B, 0x0C, 0x0E, 0x0F, 0x10, 0x12, 0x14, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x21, 0x23,
	0x25, 0x27, 0x2A, 0x2C, 0x2E, 0x31, 0x33, 0x36, 0x38, 0x3B, 0x3E, 0x40, 0x43, 0x46, 0x49, 0x4C,
	0x4E, 0x51, 0x54, 0x57, 0x5A, 0x5D, 0x60, 0x63, 0x66, 0x69, 0x6C, 0x6F, 0x73, 0x76, 0x79, 0x7C,
	0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8F, 0x92, 0x95, 0x98, 0x9B, 0x9E, 0xA1, 0xA4, 0xA7, 0xAA, 0xAD,
	0xB0, 0xB2, 0xB5, 0xB8, 0xBB, 0xBE, 0xC0, 0xC3, 0xC6, 0xC8, 0xCB, 0xCD, 0xD0, 0xD2, 0xD4, 0xD7,
	0xD9, 0xDB, 0xDD, 0xDF, 0xE1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEA, 0xEC, 0xEE, 0xEF, 0xF0, 0xF2, 0xF3,
	0xF4, 0xF5, 0xF7, 0xF8, 0xF9, 0xF9, 0xFA, 0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE,
	0xFE, 0xFE, 0xFE, 0xFE, 0xFD, 0xFD, 0xFD, 0xFC, 0xFC, 0xFB, 0xFA, 0xF9, 0xF9, 0xF8, 0xF7, 0xF5,
	0xF4, 0xF3, 0xF2, 0xF0, 0xEF, 0xEE, 0xEC, 0xEA, 0xE9, 0xE7, 0xE5, 0xE3, 0xE1, 0xDF, 0xDD, 0xDB,
	0xD9, 0xD7, 0xD4, 0xD2, 0xD0, 0xCD, 0xCB, 0xC8, 0xC6, 0xC3, 0xC0, 0xBE, 0xBB, 0xB8, 0xB5, 0xB2,
	0xB0, 0xAD, 0xAA, 0xA7, 0xA4, 0xA1, 0x9E, 0x9B, 0x98, 0x95, 0x92, 0x8F, 0x8B, 0x88, 0x85, 0x82,
};
#if (BUF_LEN < BEEP_SIN_LEN)
	#error "Size tables of sines less than the buffer sound, reduce the size of tables of sines"
#endif
/************************************************************************/
/* Поддержка генератора ШИМ												*/
/************************************************************************/
//------ Инициализация таймера битрейта
void timer1_init()
{
	TCCR1B =	(1 << WGM12) |									//Режим CTC
				(1 << CS10);									//Без деления
	TCNT1 = 0;
	OCR1A = (F_CPU/BEEP_BITRATE);								//Делитель битрейта при инициализации не важен, т.к. он перечитывается при начале воспроизведения файла
	STOP_TMR_BITRT;												//Пока таймер остановлен
}
//------- Инициализация таймера генератора ШИМ
void pwm_init()
{
	TCCR0 =
			(1<<WGM01) | (1<<WGM00) |							//Режим Fast PWM
			(0<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0 = 0;
	SOUND_OFF;
	OCR0 = 128;
	DDRB |= (1<<PORTB3);										//Ногу OC0 на вывод
}

//------- Следующая выборка из буфера. Собственно и есть процедура воспроизведения
ISR (TIMER1_COMPA_vect)
{
	if(reading0buf == BUF0_READ)								//читается из нужного буфера
		OCR0 = read_buf0[PosInBuf++];
	else
		OCR0 = read_buf1[PosInBuf++];
	if(PosInBuf == (BUF_LEN-1)) {									//Буфер прочитан
		if(reading0buf == BUF0_READ)							//Пометить соответствующий буфер как пустой
			buf0_empty = BUF_EMPTY;										
		else
			buf1_empty = BUF_EMPTY;
		reading0buf ^= 1;										//Сменить буфер
		PosInBuf = 0;												//Начать чтение сначала
	}
}

//-----------------------------------------------------------------------------------
//--------------- Работа с синусоидой, файл звука не требуется ----------------------

/************************************************************************/
/* Обновление флагов буферов для синусоиды                              */
/************************************************************************/
void BufferDownload(void){
	if (buf0_empty == BUF_EMPTY)
		buf0_empty = BUF_FULL;
	if (buf1_empty == BUF_EMPTY)
		buf1_empty = BUF_FULL;
	SetTimerTask(BufferDownload, BEEP_READ_PERIOD);
}
/************************************************************************/
/* Загрузка буфера синусом и запуск его воспроизведения                 */
/************************************************************************/
#define BEEP_SIN_COUNT_BUF	(BUF_LEN/BEEP_SIN_LEN)				//Количество умещающихся периодов синуса в буфере. 
inline void BufferSineIni(void){
	memcpy_P(read_buf0, &sin_table, BEEP_SIN_LEN);
	memcpy(read_buf0+BEEP_SIN_LEN, read_buf0, BEEP_SIN_LEN);
	memcpy(read_buf1, read_buf0, BUF_LEN);
	buf0_empty = BUF_FULL;
	buf1_empty = BUF_FULL;

	OCR1A = (F_CPU/BEEP_BITRATE)/64;
	START_TMR_BITRT;
	SOUND_ON;
	SetTimerTask(BufferDownload, BEEP_READ_PERIOD);
}

/************************************************************************/
/* Заполнение буферов звука для звука кнопок                            */
/************************************************************************/
void KeyBeepDownload(void){
	if KeyBeepIsWork{											//Звук кнопки закончен?
		SetTimerTask(KeyBeepDownload, BEEP_READ_PERIOD);		//Еще нет, продолжаем звучать
	}
	else														//Звук закончен
		SoundOff();
}
/************************************************************************/
/* Остановить воспроизведение звука кнопки                              */
/************************************************************************/
void BeepKeyOff(void){
	KeyBeepEnd;
}

/************************************************************************/
/* Настройка битрейта и ШИМ для звука кнопок, а так же для звука когда  */
/* файл звука не удается открыть										*/
/************************************************************************/
void BeebKeyboradStart(void){
	BufferSineIni();
	SetTimerTask(KeyBeepDownload, BEEP_READ_PERIOD);			//Период обновления буфера
}

/************************************************************************/
/* Выдается звук в том случае если не удалось открыть файл на SD		*/
/* карточке                                                             */
/************************************************************************/
void AlarmBeepContinue(void);									//Продолжать пищать
/************************************************************************/
/* Окончание периода включения писка, начинаем период молчания			*/
/************************************************************************/
void AlarmBeepStop(void){										//Выключить писк
	SOUND_OFF;													//Тут выключается именно вывод на ногу ШИМ, т.к. надо просто отключить звук, но оставить флаги звука в прежней позиции
	STOP_TMR_BITRT;
	if AlarmBeepIsWork											//Если сигналы будильника включены то продолжать пищать
		SetTimerTask(AlarmBeepContinue, BEEP_OFF_DELAY);		//Включить через указанный интервал
	else if (EachHourIsWork && StrikingClockCount){				//Если звук каждый час, то пискнуть еще
		StrikingClockCount--;
		SetTimerTask(AlarmBeepContinue, BEEP_OFF_DELAY);		//Включить через указанный интервал
	}
	else{
		SoundOff();												//Звук закончен выключаемся
	}
}
/************************************************************************/
/* Окончание периода молчания, начинаем период писка                    */
/************************************************************************/
void AlarmBeepContinue(void){									//Продолжать пищать
	SOUND_ON;
	START_TMR_BITRT;
	SetTimerTask(AlarmBeepStop, BEEP_ON_DELAY);					//выключить через указанный интервал
}
/************************************************************************/
/* Старт писка без карточки SD                                          */
/************************************************************************/
void AlarmBeepStart(void){
	BufferSineIni();											//Настроить ШИМ и битрейт
	SetTimerTask(AlarmBeepStop, BEEP_ON_DELAY);					//выключить через указанный интервал
}

//-----------------------------------------------------------------------------------
//--------------- Работа с файлом звука на SD-карточке ------------------------------

FRESULT FileSoundSet(const char *FileName);
/************************************************************************/
/* Чтение очередного куска звука из SD-карточки                         */
/************************************************************************/
inline FRESULT read_part(u08 *buf){
	u08 res;
	WORD rb;													//Прочитанных байт

	res = pf_read(buf, BUF_LEN, &rb);
	if (res != FR_OK)											//Ошибка чтения прекратить звук
		SoundOff();
	return res;
}

void play_file(void){

	if (CurrentChunk > DataChunkSize){							//Воспроизведение закончено
		if (AlarmBeepIsWork)									//Если будильник еще не выключен то начать воспроизведение снова
			FileSoundSet(ALARM_FILE_NAME);
		else
			if (EachHourIsWork && StrikingClockCount){			//Если звук каждый час, то воспроизвести бой
				StrikingClockCount--;
				if (FileSoundSet(ONE_HOUR_FILE_NAME) != FR_OK)	//Файл однократного боя не найден, выключить звук
					SoundOff();
			}
			else												//Все сигналы отзвучали
				SoundOff();
		return;
	}
	if(reading0buf == BUF0_READ){
		if (buf1_empty == BUF_EMPTY){
			if (read_part(read_buf1) != FR_OK)
				return;
			buf1_empty = BUF_FULL;
			CurrentChunk += BUF_LEN;
			}
	}
	else{
		if (buf0_empty == BUF_EMPTY){
			if (read_part(read_buf0) != FR_OK)
				return;
			buf0_empty = BUF_FULL;
			CurrentChunk += BUF_LEN;
		}
	}
	SetTimerTask(play_file, BEEP_READ_PERIOD);				//Период обновления буфера
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
//------- Рассчитывает битрейт используя в качестве источника данных read_buf0, возвращает RET_OK или код ошибки
inline FRESULT check_bitrate_and_stereo(void){
	int i;
	unsigned long int bitrate =0;

	if (
		(memcmp(&read_buf0[0], "RIFF", 4) == 0)					//Должна быть метка RIFF
		&& (memcmp(&read_buf0[8], "WAVE", 4) == 0)				//Должна быть метка WAVE
		&& (memcmp(&read_buf0[12], "fmt ", 4) == 0)				//Должна быть секция fmt
		&& (memcmp(&read_buf0[36], "data", 4) == 0)				//Должна быть секция data
		){
		if(read_buf0[34] != 8)									//Должна быть восмибитная дискретность
			return FR_NOT_READY;								//Не 8-и битная оцифровка
		for (i = 31; i > 27; i--) {								//читается собственно битрейт
			bitrate <<= 8;
			bitrate |= read_buf0[i];
		}
		OCR1A = (F_CPU/bitrate);								//Битрейт загоняется в делитель.
		return FR_OK;
	}
	return FR_NOT_READY;										//Это не формат wave
}

/************************************************************************/
/* Настройка битрейта, ШИМ и прочего для воспроизведения звукового файла*/
/* FileName - имя файла, возвращает RET_OK если все нормально или код	*/
/* ошибки если что-то пошло не так										*/
/************************************************************************/
FRESULT FileSoundSet(const char *FileName){

	FRESULT Ret;
	WORD rb;												//Прочитанных байт

	if (pf_disk_is_mount() != FR_OK){						//Карта еще не определялась, пробуем ее инициализировать.
		Ret = pf_mount(&fs);
		if (Ret != FR_OK) return Ret;
	}
	Ret = pf_open(FileName);
	if (Ret != FR_OK) return Ret;
	Ret = pf_lseek (0);
	if (Ret != FR_OK) return Ret;
	Ret = pf_read(&read_buf0, 44, &rb);
	if (Ret != FR_OK) return Ret;
	if (rb != 44) return FR_NOT_OPENED;						//Неправильное чтение заголовка
	Ret = check_bitrate_and_stereo();						//Чтение заголовока
	if (Ret != FR_OK) return Ret;
	memcpy(&DataChunkSize, &read_buf0[40], 4);				//Количество выборок
	Ret = pf_lseek(44);										//Ставим на начало секции выборок
	if (Ret != FR_OK) return Ret;
	Ret = read_part(read_buf0);								//Начало воспроизведения
	if (Ret != FR_OK) return Ret;
	buf0_empty = BUF_FULL;
	buf1_empty = BUF_EMPTY;
	CurrentChunk = BUF_LEN;
	PosInBuf = 0;
	reading0buf = BUF0_READ;
	SetTimerTask(play_file, BEEP_READ_PERIOD);				//Период обновления буфера
	START_TMR_BITRT;
	SOUND_ON;
	return FR_OK;
}

//-----------------------------------------------------------------------------------
/************************************************************************/
/* Включение звука определенного типа                                   */
/************************************************************************/
void SoundOn(u08 SoundType){								//Включить сигнал указанного типа

	switch (SoundType)
	{
		case SND_KEY_BEEP:									//Звук кнопок
			if KeyBeepSettingIsOn{							//Если включен то пищать
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtButton);				//Запустить регулировку громкости
#endif				
				KeyBeepBegin;
				BeebKeyboradStart();						//Настроить ШИМ и битрейт
				SetTimerTask(BeepKeyOff, BEEP_KEY_DELAY);	//Длительность звучания звука кнопки
			}
			break;
		case SND_ALARM_BEEP:								//Звук будильника
			if AlarmBeepIsNoWork{							//Запускать звук только если он еще не включен
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtAlarm);
#endif
				AlarmBeepBegin;								//Флаг звука будильника взвести
				if (FileSoundSet(ALARM_FILE_NAME) != FR_OK)
					AlarmBeepStart();						//Не удалось воспроизвести звук с карточки, запускаем обычную пищалку
			}
			break;
		case SND_EACH_HOUR:									//Звук курантов
			if EachHourSettingIsOn{							//Звук курантов включен?
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtEachHour);
#endif
				EachHourBegin;								//Флаг звука курантов взвести
				StrikingClockCount = HourToInt(Watch.Hour);	//Количество ударов курантов
				if (StrikingClockCount>12)					//Если больше полудня то привести к дополуденному бою
					StrikingClockCount -= 12;
				if (FileSoundSet(EACH_HOUR_FILE_NAME) != FR_OK)
					AlarmBeepStart();						//Не удалось воспроизвести звук с карточки, запускаем обычную пищалку
			}
			break;
	
		case SND_SENSOR_TEST:
		case SND_TEST:										//Выдать тестовый звук для демонстрации громкости
#ifdef VOLUME_IS_DIGIT
			if (SoundType == SND_SENSOR_TEST)
				VolumeAdjustStart(vtSensorTest);
			else
				VolumeAdjustStart(vtButton);
#endif
			KeyBeepBegin;
			BeebKeyboradStart();							//Настроить ШИМ и битрейт
			SetTimerTask(BeepKeyOff, BEEP_KEY_DELAY);		//Длительность звучания звука кнопки
			break;
		case SND_TEST_EACH:									//Тест звука курантов
#ifdef VOLUME_IS_DIGIT
			VolumeAdjustStart(vtEachHour);
#endif
			EachHourBegin;									//Флаг звука курантов взвести
			StrikingClockCount = 1;							//Количество ударов курантов
			if (FileSoundSet(EACH_HOUR_FILE_NAME) != FR_OK)
				AlarmBeepStart();							//Не удалось воспроизвести звук с карточки, запускаем обычную пищалку
			break;
		case SND_TEST_ALARM:								//Тест звука будильника
#ifdef VOLUME_IS_DIGIT
			VolumeAdjustStart(vtAlarm);
#endif
			AlarmBeepBegin;									//Флаг звука будильника взвести
			if (FileSoundSet(ALARM_FILE_NAME) != FR_OK)
				AlarmBeepStart();							//Не удалось воспроизвести звук с карточки, запускаем обычную пищалку
			break;
		default:
			break;
	}
}

/************************************************************************/
/* Выключить сигнал                                                     */
/************************************************************************/
void SoundOff(void){
#ifdef VOLUME_IS_DIGIT
	VolumeOff();
#endif
	SOUND_OFF;								//Выключить подачу ШИМ на ногу OC
	STOP_TMR_BITRT;							//Выключить таймер битрейта
	buf0_empty = BUF_EMPTY;					//Все буферы пусты
	buf1_empty = BUF_EMPTY;
	KeyBeepEnd;
	EachHourEnd;
	AlarmBeepEnd;
}

/************************************************************************/
/* Выключить куранты                                                    */
/************************************************************************/
void EachHourSettingSetOff(void){
	EachHourSettingOff;
}

/************************************************************************/
/* Включить-выключить сигнал каждый час                                 */
/************************************************************************/
void EachHourSettingSwitch(void){
	if EachHourSettingIsOn
		EachHourSettingOff;
	else
		EachHourSettingOn;
	if (Refresh != NULL)
		Refresh();
}
/************************************************************************/
/* Проверить флаг звучания сигнала каждый час. 0 - выключен             */
/************************************************************************/
u08 EachHourSettingIsSet(void){
	if EachHourSettingIsOn
		return 1;
	else
		return 0;
}

/************************************************************************/
/* Выключить звук кнопок                                                */
/************************************************************************/
void KeyBeepSettingSetOff(void){
	KeyBeepSettingOff;
}

/************************************************************************/
/* Включить-выключить сигнал нажатия кнопок                             */
/************************************************************************/
void KeyBeepSettingSwitch(void){
	if KeyBeepSettingIsOn
		KeyBeepSettingOff;
	else
		KeyBeepSettingOn;
	if (Refresh != NULL)
		Refresh();
}

/************************************************************************/
/* Проверить флаг звучания кнопок. 0 - звук кнопок выключен             */
/************************************************************************/
u08 KeyBeepSettingIsSet(void){
	if KeyBeepSettingIsOn
		return 1;
	else
		return 0;
}

/************************************************************************/
/* Сигнал будильника в текущий момент звучит? 0 - нет, 1 - да           */
/************************************************************************/
u08 AlarmBeepIsSound(void){
	if AlarmBeepIsWork
		return 1;
	else
		return 0;
}

u08 SoundIsBusy(void){
	return (SoundFlag & (Bit(EACH_HOUR_BEEP_WORK) | Bit(KEY_BEEP_WORK) | Bit(ALARM_BEEP_WORK)))?1:0;
}

/************************************************************************/
/* Тестирует наличие SD-карты в слоту и при обнаружении проводит попытку*/
/* инициализации. Результат выводится на дисплей. 						*/
/************************************************************************/
void SD_Card_Test(void){
	unsigned char static NumberOfAttempt = 0;					//Количество попыток монтирования карточки
	
	if NoSDCard(){												//Карты в слоту нет
		pf_mount(0);											//Отмонтировать карту
		NumberOfAttempt = 0;									//Попытки монтирования 0
		SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);
	}
	else{														//Карта в слоту
		if (pf_disk_is_mount() != FR_OK){						//Карта еще не определялась
			if (NumberOfAttempt < (SD_CARD_ATTEMPT_MAX+1)){		//Количество попыток монтирования карты еще не закончено, будем пытатся еще примонтировать
				if (NumberOfAttempt){							//защитный интервал был отработан, начинаем монтирование
					if (pf_mount(&fs) == FR_OK)					//Удачно примонтировано
						SetClockStatus(csSDCardDetect, ssSDCardOk);//Удачная инициализация выводим сообщение
					else										//Монтирование неудачно
						if (NumberOfAttempt == SD_CARD_ATTEMPT_MAX)//Попытки исчерпаны, выводим сообщение
							SetClockStatus(csSDCardDetect, ssSDCardNo);
					SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);	//Продолжается отслеживание датчика наличия карты
				}
				else{
					SetTimerTask(SD_Card_Test, SD_WHITE_PERIOD);
				}
				NumberOfAttempt++;								//Попытка выполнена
			}
		}
		else{
			SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);			//Продолжается отслеживание датчика наличия карты
		}
	}
}

/************************************************************************/
/* Инициализация звуковой системы                                       */
/************************************************************************/
void SoundIni(void){
	pf_mount(0);												//Отмонтировать карту
	timer1_init();												//Инициализация таймера обеспечивающего битрейт
	pwm_init();													//Инициализация таймера обеспечивающего амплитуду аналогового сигнала - скважиность ШИМ
	SD_Card_Test();												//Проверка наличия карты в слоту
	KeyBeepSettingOff;
	EachHourSettingOff;
#ifdef VOLUME_IS_DIGIT											//Запустить регулятор громкости если он есть
	VolumeIni();
#endif
}
