/************************************************************************/
/* Часы	на матричных индикаторах                                        */
/* Настройки на конкретную схему в файле Clock.h                        */
/* Andy Korg (c) 2014 г.												*/
/* http://radiokot.ru/circuit/digital/home/199/							*/
/************************************************************************/

#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <stddef.h>
#include <stdlib.h>

#include "IIC_ultimate.h"						//Обеспечение протокола I2C
#include "i2c.h"								//Интерфейс работы с устройствами на I2C шине, в настоящее время только RTC
#include "avrlibtypes.h"
#include "bits_macros.h"
#include "EERTOSHAL.h"
#include "EERTOS.h"
#include "Clock.h"								//Всякие определения для часов
#include "CalcClock.h"							//Расчеты для часов
#include "Display.h"							//Вывод в буфер дисплея
#include "keyboard.h"							//Обслуживание клавиатуры
#include "Alarm.h"								//Обеспечение работы будильников
#include "Sound.h"								//Работа с SD-картой и звуком
#include "eeprom.h"								//Сохранение настроек часов
#include "IRrecive.h"							//Прием команд от ИК-пульта управления
#include "sensors.h"							//Обработка результатов измерения датчиков
#include "ds18b20.h"							//Обслуживание термодатчика ds18b20
#include "nRF24L01P.h"							//Радиомодуль
#include "esp8266hal.h"							//Модуль работы с WiFi
#ifdef VOLUME_IS_DIGIT
#include "Volume.h"
#endif

volatile struct sClockValue	Watch;				//Текущее значение часов в RTC и различные флаги состояния автоматов в переменной Mode
volatile enum tClockStatus ClockStatus;			//Текущий режим часов
volatile enum tSetStatus SetStatus;				//Текущий подрежим установки
volatile struct sClockValue *CurrentCount;		//Отображаемый на дисплее счетчик - часы или будильники
struct sAlarm *CurrentShowAlarm;				//Текущий будильник
u08 CurrentAlarmDayShow,						//Текущий день недели для текущего будильника
	CurrentSensorShow,							//Текущий настраиваемый сенсор
	CurrentSensorAlphaNumAndIrCode;				//Текущий номер настраиваемого символа в имени сенсора и индекс в массиве кодово клаиш ИК пульта для экономии ОЗУ
volatile u08 CountWaitingEndSet;				//Отсчет промежутка времени в течении которого не было нажатий клавиш или срабатывания от ИК датчика в не основном режиме. Используется для возврата из всяких установок

VOID_PTR_VOID Refresh = NULL;					//Указатель на функцию которую можно взывать для обновления текущего экрана

const u08 PROGMEM								//Фразы выводимые в разных режимах
	setting_word[] = "установка\xa0часов",
	alarm_set_word[] =	"будильник–установка",	//внимание!- используется маленький минус
	tune_set_word[] = "часы\xa0настройка",
	tune_HZspeed_word[] = "скорость бегущей строки  ",
	tune_sound_word[] = "звук\xa0настройка",
	tune_font_word[] =	"шрифт\xa0часов",
	each_hour_word[] = "куранты\xa0",
	key_beep_word[] = "звук\xa0кнопок\xa0",

	sens_set_word[] = "датчики\xa0настройка",
	sens_set_name_word[] = "имя\xa0датчика",
	sens_set_adr_word[] = "адрес\xa0датчика",

	sd_card_ok_word[] = "старт",
	sd_card_err_word[] = "ошибка",
	sd_card_word[] = " карты",

	const_alrm_vol_word[] = "будильник-постоянная громкость",
	inc_alrm_vol_word[] = "будильник нарастающая громкость до ",
	max_alrm_vol_word[] = "максимума",
	lvl_alrm_vol_word[] = "уровня",
	inet_start_word[] = "взять время из интернета",
	inet_loaded_word[] = "время загружено",
	inet_tune_word[] = "сеть настройка",
	ip_address[] = "ip адрес часов ",

#ifndef IR_SAMSUNG_ONLY
	ir_set_word[] = "пульт ду-настройка",
	ir_set_code_name[][4] = {					//Порядок слов в этом массиве должен соответствовать порядку команд в массиве IRcmdList
		" ok",									//Команда меню ОК
		"stp",									//Команда меню Step
		"inc",									//Увеличить текущий счетчик
		"dec",									//Уменьшить текущий счетчик
		"ply",									//Команда воспроизвести мелодию будильника
		"end"									//остановить
	},
#endif
	on_word[] =	"включено",
	off_word[] = "выключено"
	;
	

/************************************************************************/
/* Выводится фраза в бегущую строку неограниченное число раз            */
/* Start = 1 - сначала подготовить дисплей, 0 - только добавить фразу	*/
/************************************************************************/
#define PHRAZE_START	1
#define PHRAZE_CONTINUE	0

void WordPut(const u08 *Value, u08 Start){
	u08 i;

	if (Start){
		ClearDisplay();
		CreepInfinteSet();
	}
	for (i=0; pgm_read_byte(&Value[i]) != 0;i++){
		sputc(pgm_read_byte(&Value[i]), UNDEF_POS);
		sputc(S_SPICA, UNDEF_POS);				//промежуток разделяющий буквы
	}
}


/************************************************************************/
/* Включить мигание определенными знакоместами                          */
/************************************************************************/
#define NO_FLASH	0							//Выключить мигание всех знакомест
#define HOUR_FLASH	1							//Мигать знакоместами часов
#define MINUT_FLASH	2							//Мигать знакоместами минут
#define ALL_FLASH	3							//Мигать всеми знакоместами
#define UNDEF_FLASH	4							//Не изменять флаги мигания

void FlashSet(u08 Value){
	switch (Value){
		case NO_FLASH:
			sputc(S_FLASH_OFF, DIGIT3);			//Выключить мигание
			sputc(S_FLASH_OFF, DIGIT2);
			sputc(S_FLASH_OFF, DIGIT1);
			sputc(S_FLASH_OFF, DIGIT0);
			break;
		case HOUR_FLASH:
			sputc(S_FLASH_ON, DIGIT3);			//Мигать разрядами часов
			sputc(S_FLASH_ON, DIGIT2);
			sputc(S_FLASH_OFF, DIGIT1);
			sputc(S_FLASH_OFF, DIGIT0);
			break;
		case MINUT_FLASH:
			sputc(S_FLASH_OFF, DIGIT3);			//Мигать разрядами минут
			sputc(S_FLASH_OFF, DIGIT2);
			sputc(S_FLASH_ON, DIGIT1);
			sputc(S_FLASH_ON, DIGIT0);
			break;
		case ALL_FLASH:
			sputc(S_FLASH_ON, DIGIT3);			//Мигать всеми разрядами
			sputc(S_FLASH_ON, DIGIT2);
			sputc(S_FLASH_ON, DIGIT1);
			sputc(S_FLASH_ON, DIGIT0);
		default:
			break;
	}
}

/************************************************************************/
/* Вывод мигающей фразы "ждем"                                          */
/************************************************************************/
void PhrazeWaitShow(void){
	FlashSet(ALL_FLASH);
	sputc('ж', DIGIT3);
	sputc('д', DIGIT2);
	sputc('е', DIGIT1);
	sputc('м', DIGIT0);
}

/************************************************************************/
/* Вывести состояние датчика в режиме тестирования						*/
/************************************************************************/
void SensotTestShow(void){

	ClearDisplay();
	for(u08 i=0; i<SENSOR_MAX;i++){
		if SensorTestModeIsOn(SensorFromIdx(i)){
			u08 SensAdr = bin2bcd_u32(SensorFromIdx(i).Adr>>1, 1);
			u08 SensVal = bin2bcd_u32(SensorFromIdx(i).Value, 1);
			sputc(Tens(SensAdr), DIGIT3);
			sputc(Unit(SensAdr), DIGIT2);
			if (SensorBatareyIsLow(SensorFromIdx(i))){	//Низкий заряд батареи
				sputc('б', DIGIT1);						//батареи
				sputc('н', DIGIT0);						//нет
			}
			else if (SensorIsNo(SensorFromIdx(i))){//Сам датчик в устройстве не найден
				sputc('д', DIGIT1);						//датчика
				sputc('н', DIGIT0);						//нет
			}
			else{										//Все нормально, выводится текущее значение датчика
				sputc(Tens(SensVal), DIGIT1);
				sputc(Unit(SensVal), DIGIT0);
			}
			break;	
		}
	}
}

/************************************************************************/
/* Показать состояние сигнала каждый час                                */
/************************************************************************/
void EachHourBeepShow(void){
	ClearDisplay();
	WordPut(each_hour_word, PHRAZE_START);
	if (EachHourSettingIsSet())
		WordPut(on_word, PHRAZE_CONTINUE);
	else
		WordPut(off_word, PHRAZE_CONTINUE);
}

/************************************************************************/
/* Показать состояние флага срабатывания клавиш                         */
/************************************************************************/
void KeyBeepShow(void){
	ClearDisplay();
	WordPut(key_beep_word, PHRAZE_START);
	if (KeyBeepSettingIsSet())
		WordPut(on_word, PHRAZE_CONTINUE);
	else
		WordPut(off_word, PHRAZE_CONTINUE);
}

#ifdef VOLUME_IS_DIGIT
void VolumeShow(u08 Vol, u08 VolName1, u08 VolName0){
	u32 i = bin2bcd_u32(Vol, 1);
	
	sputc(VolName1, DIGIT3);
	sputc(VolName0, DIGIT2);
	sputc(Tens((u08)i), DIGIT1);
	sputc(Unit((u08)i), DIGIT0);
	FlashSet(MINUT_FLASH);
	SoundOn(SND_TEST);
}

/************************************************************************/
/* Показать громкость звука нажатой клавиши								*/
/************************************************************************/
void VolumeBeepShow(void){
	VolumeShow(VolumeClock[vtButton].Volume, 'K', 'Н');
}

/************************************************************************/
/* Показать громкость звука ежечастного сигнала							*/
/************************************************************************/
void VolumeEachHourShow(void){
	VolumeShow(VolumeClock[vtEachHour].Volume, 'Е', 'Ч');
}

/************************************************************************/
/* Показать громкость звука будильника									*/
/************************************************************************/
void VolumeAlarmShow(void){
	VolumeShow(VolumeClock[vtAlarm].Volume, 'Б', 'У');
}

/************************************************************************/
/* Показать режим регулирования громкости будильника                    */
/************************************************************************/
void VolumeAlarmIncShow(void){
	if (VolumeClock[vtAlarm].LevelVol == vlConst){				//Режим постоянного уровня громкости
		WordPut(const_alrm_vol_word, PHRAZE_START);
	}
	else{
		WordPut(inc_alrm_vol_word, PHRAZE_START);				//Режим нарастающего уровня
		if (VolumeClock[vtAlarm].LevelVol == vlIncreas)			//до уровня
			WordPut(lvl_alrm_vol_word, PHRAZE_CONTINUE);
		else
			WordPut(max_alrm_vol_word, PHRAZE_CONTINUE);		//до максимального уровня
	}
}
#endif

/************************************************************************/
/* Показать день недели и режим включения - выключения будильника       */
/*  в этот день.														*/
/************************************************************************/
void AlarmNumDayShow(void){
	PlaceDay(CurrentAlarmDayShow, PLACE_HOURS);
	sputc((AlrmDyIsOn(*CurrentShowAlarm, CurrentAlarmDayShow))?'+':'-', DIGIT1);
	sputc(BLANK_SPACE, DIGIT0);
}

/************************************************************************/
/* Показать номер будильника                                            */
/************************************************************************/
void AlarmNumShow(void){
	sputc('Б', DIGIT3);
	sputc(CurrentShowAlarm->Id, DIGIT2);
	if (LitlNumAlarm())
		sputc('е', DIGIT0);
	else
		sputc('д', DIGIT0);
}

/************************************************************************/
/* Показать длительность сигнала будильника                             */
/************************************************************************/
void AlarmDurationShow(void){
	u08 Du = bin2bcd_u32(AlarmDuration(*CurrentShowAlarm), 1);
	
	AlarmNumShow();
	sputc(Tens(Du), DIGIT1);
	sputc(Unit(Du), DIGIT0);
}

/************************************************************************/
/* Показать номер будильника в режиме установки вкл-выкл будильника     */
/************************************************************************/
void AlarmNumSetShow(void){
	AlarmNumShow();
	if AlarmIsOn(*CurrentShowAlarm)
		sputc('+', DIGIT1);
	else
		sputc('-', DIGIT1);
}

/************************************************************************/
/* Показать год                                                         */
/************************************************************************/
void ShowYear(void){
	FlashSet(ALL_FLASH);
	sputc(Tens(CENTURY_DEF), DIGIT3);
	sputc(Unit(CENTURY_DEF), DIGIT2);
	sputc(Tens((*CurrentCount).Year), DIGIT1);
	sputc(Unit((*CurrentCount).Year), DIGIT0);
}

/************************************************************************/
/* Вывод значения температуры.											*/
/*  либо в бегущую строку либо в фиксированные знакоместа				*/
/************************************************************************/
void TemperatureShow(u08 Value, u08 Fixed){
	u08 Place1 = UNDEF_POS, Place2 = UNDEF_POS, Place3 = UNDEF_POS, Place0 = UNDEF_POS;
	if (Fixed != UNDEF_POS){
		Place1 = DIGIT1;
		Place2 = DIGIT2;
		Place3 = DIGIT3;
		Place0 = DIGIT0;
	}
	if (Value & 0x80){									//Если отрицательная то в дополнительном коде
		sputc('–', Place3);
		Value = ~Value;
	}
	else
		sputc('+', Place3);
	Value = bin2bcd_u32(Value & 0x7f, 1);
	sputc(Tens(Value), Place2);
	sputc(Unit(Value), Place1);
	sputc('°', Place0);										// знак градуса
}


/************************************************************************/
/* Вывод значения датчика	                                           */
/************************************************************************/
void SensorCreepShow(char *Nam, u08 Value){
	
	sputc(' ', UNDEF_POS);
	for(u08 j=0; j < SENSOR_LEN_NAME; j++){		//Имя датчика
		if (Nam[j]){
			sputc(Nam[j], UNDEF_POS);
			sputc(S_SPICA, UNDEF_POS);
		}
		else
			break;
	}
	sputc(S_SPICA, UNDEF_POS);
	if (Value != TMPR_NO_SENS){					//Температура действительна
		TemperatureShow(Value, UNDEF_POS);
		sputc('C', UNDEF_POS);					// большая C
	}
	else{
		sputc('-', UNDEF_POS);
		sputc('-', UNDEF_POS);
	}
	sputc(S_SPICA, UNDEF_POS);
}

/************************************************************************/
/* Дата с днем недели бегущей строкой и прочими добавочными данными     */
/************************************************************************/
void ShowDate(void){
	
	u08 i;
	
	AddNameWeekFull(what_day(Watch.Date, Watch.Month, Watch.Year));	//Имя дня недели
	sputc(' ', UNDEF_POS);
	sputc(S_SPICA, UNDEF_POS);
	if (Tens(Watch.Date))
		sputc(Tens(Watch.Date), UNDEF_POS);			//Дата месяца
	sputc(S_SPICA, UNDEF_POS);
	sputc(Unit(Watch.Date), UNDEF_POS);
	sputc(' ', UNDEF_POS);
	AddNameMonth(Watch.Month);						//Название месяца
	for(i=0; i<SENSOR_MAX; i++){					//Вывод результатов для датчиков
		if (SensorIsShow(SensorFromIdx(i))){
			if (SensorFromIdx(i).SleepPeriod == 0)	//сообщения от датчика не были получены в течение длительного периода
				SensorCreepShow(SensorFromIdx(i).Name, TMPR_NO_SENS);
			else
				SensorCreepShow(SensorFromIdx(i).Name, (SensorIsNo(SensorFromIdx(i)))?TMPR_NO_SENS:SensorFromIdx(i).Value);
		}
	}
}

/************************************************************************/
/* Показать дату один раз, Если вызывается еще раз во время показа      */
/* бегущей строки то вызывается функция указанная в GetSecondFunc		*/
/************************************************************************/
void StartDateShow(void){
	if (CreepCount() == 1)
		GetSecondFunc();
	else{
		ClearDisplay();
		CreepOn(1);										//Бегущая строка
		ShowDate();
	}
}

/************************************************************************/
/* Дата для установки - номер дня месяца и номер месяца                 */
/************************************************************************/
void ShowDateSet(void){
	sputc(Tens((*CurrentCount).Date), DIGIT3);	//Дата
	sputc(Unit((*CurrentCount).Date), DIGIT2);
	sputc(Tens((*CurrentCount).Month), DIGIT1);
	sputc(Unit((*CurrentCount).Month), DIGIT0);
}

/************************************************************************/
/* Вывод времени из текущего счетчика часов								*/
/************************************************************************/
void TimeOutput(void){
	if (((*CurrentCount).Hour & 0xf0) == 0)				//не выводить не значащий ноль
		sputc(BLANK_SPACE, DIGIT3);
	else
		sputc(Tens((*CurrentCount).Hour), DIGIT3);
	sputc(Unit((*CurrentCount).Hour), DIGIT2);
	sputc(Tens((*CurrentCount).Minute), DIGIT1);
	sputc(Unit((*CurrentCount).Minute), DIGIT0);
}

/************************************************************************/
/* Вывод текущего времени и даты на каждой SECOND_DAY_SHOW              */
/************************************************************************/
void ShowTime(void){
	static u08 StartDateShow = 0;
	
	if (!Watch.Second)									//Обнуление счетчика старта вывода даты каждую минуту
		StartDateShow = 0;
	StartDateShow++;
	if ((StartDateShow == SECOND_DAY_SHOW) && CreepIsOff()) {//Пора запустить бегущую строку с датой и она еще не запущена
		if (espInstalled()){							//Если модуль esp установлен, то сначала запросить его на предмет произвольной строки
			u08 i=0;
			espUartTx(ClkRead(CLK_CUSTOM_TXT), &i, 1);//После получения команды чтения модуль отвечает записью произвольного текста. Эта команда обрабатывается специальным образом. см.ISR(ESP_UARTRX_vect) в файле esp8266hal.c
		}
		else{
			ClearDisplay();
			CreepOn(1);
			ShowDate();
		}
	}
	else{
		if CreepIsOn() return;							//Ожидается окончание прокрутки бегущей строки
		VertSpeed = VERT_SPEED;							//Нормальная скорость вертикального скроллирования
		if (ClockStatus == csAlarm)						//В режиме сработавшего будильника мигать всеми индикаторами
			FlashSet(ALL_FLASH);
		else
			FlashSet(NO_FLASH);
		TimeOutput();
		if (OneSecond){									//Мигающая точка внизу
			plot(11,7,1);
			plot(12,7,0);
		}
		else{
			plot(11,7,0);
			plot(12,7,1);
		}
	}
}

/************************************************************************/
/* Вывести часы на дисплей в режиме выбора шрифта                       */
/************************************************************************/
void FontShow(void){
	FlashSet(ALL_FLASH);
	TimeOutput();
}

/************************************************************************/
/* Вывод минут-секунд		                                            */
/************************************************************************/
void ShowSecond(void){
	CreepOff();
	FlashSet(NO_FLASH);
	sputc(Tens(Watch.Minute), DIGIT3);
	sputc(Unit(Watch.Minute), DIGIT2);
	sputc(Tens(Watch.Second), DIGIT1);
	sputc(Unit(Watch.Second), DIGIT0);
	if (OneSecond){											//Мигающая точка внизу
		plot(11,7,1);
		plot(12,7,0);
	}
	else{
		plot(11,7,0);
		plot(12,7,1);
	}
}

/********************************** ДАТЧИКИ **************************************/
/************************************************************************/
/* Вывод на дисплей номера датчика для настройки                        */
/************************************************************************/
void SensorsNumShow(void){
	sputc('д', DIGIT3);
	sputc(CurrentSensorShow, DIGIT2);
	if SensorIsShow(SensorFromIdx(CurrentSensorShow))
		sputc('+', DIGIT1);
	else
		sputc('-', DIGIT1);
}

/************************************************************************/
/* Вывод адреса датчика на шине для настраиваемого датчика              */
/************************************************************************/
void SensorsAdrShow(void){
	sputc('д', DIGIT3);
	sputc(CurrentSensorShow, DIGIT2);
	sputc('а', DIGIT1);
	sputc(SensorFromIdx(CurrentSensorShow).Adr>>1, DIGIT0);
}

/************************************************************************/
/* Вывод на дисплей имени датчика для настройки                         */
/************************************************************************/
void SensorsAlphaShow(void){
	sputc(CurrentSensorShow, DIGIT3);
	sputc(SensorFromIdx(CurrentSensorShow).Name[0], DIGIT2);
	sputc(SensorFromIdx(CurrentSensorShow).Name[1], DIGIT1);
	sputc(SensorFromIdx(CurrentSensorShow).Name[2], DIGIT0);
}

/************************************************************************/
/* Выбор следующего датчика для отображения                             */
/************************************************************************/
void SensSetNext(void){
	CurrentSensorShow++;
	if (CurrentSensorShow == SENSOR_MAX)
		CurrentSensorShow = 0;
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Выключение-выключение текущего датчика								*/
/************************************************************************/
void SensSwitch(void){
	if SensorIsShow(SensorFromIdx(CurrentSensorShow))
		SensorShowOff(SensorFromIdx(CurrentSensorShow));
	else
		SensorShowOn(SensorFromIdx(CurrentSensorShow));
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Проверка состояния датчика.											*/
/************************************************************************/
u08 SensTestON(void){
	return SensorIsShow(SensorFromIdx(CurrentSensorShow))?1:0;
}

/************************************************************************/
/* Перебираются адреса датчика на шине                                  */
/************************************************************************/
inline u08 SensAdrNotValid(u08 Value){
	for(u08 i=0;i<SENSOR_MAX;i++)
		if ((SensorFromIdx(i).Adr == Value) && (i != CurrentSensorShow))
			return 1;											//Такой адрес уже есть в массиве датчиков, пропустить его
	return 0;													//Такого адреса еще нет 	
}
void SensAdrSet(void){
	do{
		SensorFromIdx(CurrentSensorShow).Adr += 0b10;
		if (SensorFromIdx(CurrentSensorShow).Adr == (SENSOR_ADR_MASK+0b10))
			SensorFromIdx(CurrentSensorShow).Adr = 0;
	} while (SensAdrNotValid(SensorFromIdx(CurrentSensorShow).Adr));
	if (SensorFromIdx(CurrentSensorShow).Adr == SENSOR_DS18B20)		//Для датчика на шине 1-ware сразу запускаем измерение
		StartMeasureDS18();
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Перебираются буквы в текущем знакоместе для набора имени датчика     */
/* Выводятся только русские буквы от 0xc0 до 0xdf и точка				*/
/************************************************************************/
void SensNameSet(void){
	u08 val = SensorFromIdx(CurrentSensorShow).Name[CurrentSensorAlphaNumAndIrCode]; //Выведено в переменную только лишь для читабельности

	if (val == 0)												//Конец строки, следующий символ пробел
		val = S_SPACE;
	else if (val == S_SPACE)									//Пробел, следующая цифра 0
		val = 0x30;
	else if (val == 0x39)										//Цифра 9, следующая точка
		val = S_DOT;											
	else if (val == S_DOT)										//Была точка, буква а
		val = CYRILLIC_BEGIN_CAPITAL;
	else if (val == CYRILLIC_END_CAPITAL)						//Буква я, следующий пробел
		val = S_SPACE;
	else														//Просто следующий символ
		val++;
	SensorFromIdx(CurrentSensorShow).Name[CurrentSensorAlphaNumAndIrCode] = val;
	if (Refresh != NULL)
		Refresh();												//Обновить экран
}

/************************************************************************/
/* Отслеживается наличие датчика, при появлении выводится измеренная	*/
/* температура, пока датчика нет выводится сообщение "ждем"				*/
/************************************************************************/
void SensWait(void){
	//u08 Tempr = SensorFromIdx(CurrentSensorShow).Value;

	ClearDisplay();
	if SensorIsSet(SensorFromIdx(CurrentSensorShow)){			//Есть какой-то ответ (от радиодатичка может быть ответ, но сам датчик может не ответить)
		if (SensorFromIdx(CurrentSensorShow).Value != TMPR_NO_SENS){								//Какой-то законный ответ
			TemperatureShow(SensorFromIdx(CurrentSensorShow).Value, DIGIT0);
		}
		else{
			sputc('p',DIGIT3);									//Есть сигнал от радиодатичка, но нет ответа от самого датчика
			sputc(' ',DIGIT2);
			sputc('e',DIGIT1);
			sputc('r',DIGIT0);
		}
	}
	else
		PhrazeWaitShow();
}

#ifndef IR_SAMSUNG_ONLY
/************************************************************************/
/* Функция приема кода от пульта ДУ в режиме записи кода клавиши        */
/************************************************************************/
void IRRecivCode(u08 AdresIR, u32 CommandIR){
	if (CommandIR){
		if (AdresIR == REMOT_TV_ADR){							//Так то эта  проверка лишняя так как вызов функции настроен с применением константы но пусть будет
			IRcmdList[CurrentSensorAlphaNumAndIrCode] = CommandIR;
			TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);	//Взвести счетчик таймаута возврата в основной режим
		}
		sputc(S_FLASH_ON, DIGIT0);
		if (Refresh != NULL)
			Refresh();
	}
	SetTimerTask(IRStartFromDelay, SCAN_PERIOD_KEY);			//Включение ИК приемника после защитного интервала
}

/************************************************************************/
/* Вывод состояния кода команды ИК пульта                               */
/************************************************************************/
void IRCodeShow(void){

	u08 x = CellStart(DIGIT0);
	
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][0]), DIGIT3);
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][1]), DIGIT2);
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][2]), DIGIT1);
	if (IRcmdList[CurrentSensorAlphaNumAndIrCode]){				//Есть код. Выводится его "рисунок"
		for (u08 byt = 0; byt != IR_NUM_BYTS; x++, byt++)
			for(u08 y = 0; y != 8; y++)
				plot(x,y, BitIsSet(((u08)(IRcmdList[CurrentSensorAlphaNumAndIrCode] >> MULTIx8(byt)) & 0xff), y)?1:0);
	}
	else
		sputc(S_MINUS, DIGIT0);									//Нет кода
}

/************************************************************************/
/* Записать текущий код					                                */
/************************************************************************/
void IRCodeSave(void){
	if (Refresh != NULL)
		Refresh();
	if (IRcmdList[CurrentSensorAlphaNumAndIrCode] != 0)			//Записать можно только существующий код
		FlashSet(NO_FLASH);
}

/************************************************************************/
/* Выбор следующего кода команды для пульта ДУ	                        */
/************************************************************************/
u08 IRNextCode(void){
	CurrentSensorAlphaNumAndIrCode++;
	if (CurrentSensorAlphaNumAndIrCode == IR_NUM_COMMAND)
		return 1;
	if (Refresh != NULL)
		Refresh();
	return 0;
}

#endif	//IR_SAMSUNG_ONLY

/************************************************************************/
/* Вывод IP адреса в режиме station                                     */
/************************************************************************/
void IP_Show(void){
	WordPut(ip_address, PHRAZE_START);
	if (espStationIP != NULL){
		for (u08 i=0; *(espStationIP+i);i++){					//Вывод адреса
			sputc(*(espStationIP+i), UNDEF_POS);
			sputc(S_SPICA, UNDEF_POS);							//промежуток разделяющий буквы
		}
		free(espStationIP);
		espStationIP = NULL;
	}
	else{														//Адрес еще не запрашивался, запрашиваем	
		espGetIPStation();
	}
}

/************************************************************************/
/* Функция обработки события каждую нулевую секунду в минуте            */
/* Обеспечивает запуск будильников и таймаута возврата в основной режим */
/************************************************************************/
void Check0SecondEachMinut(void){
	TimeoutReturnToClock(0);									//Проверить необходимость возврата в основной режим. Желательно что бы эта проверка была первой, что бы уже из основного режима вызывались все остальные
	AlarmCheck();												//Проверить необходимость запуска будильников и запустить их при необходимости
}

/************************************************************************/
/* Небольшой кусок кода вынесенный для уменьшения объема кода		    */
/************************************************************************/
void SettingProcess(SECOND_RTC Func, u08 Flash){
	ClearDisplay();												//очистить экран
	VertSpeed = VERT_SPEED_FAST;								//Быстрая смена позиций
	Func();														//Обновить изображение
	if (Flash != UNDEF_FLASH){
		FlashSet(NO_FLASH);										//Выключить мигание знакомест
		if (Flash != NO_FLASH)
			FlashSet(Flash);									//Требуемые знакоместа мигать
	}
	Refresh = Func;												//Определить функцию для обновления экрана
}

/************************************************************************/
/* Установка счетчика времени и даты                                    */
/************************************************************************/
void TimeDateCountSet(enum tSetStatus Value){
	u08 Flash = MINUT_FLASH, ctrlCount = clkadrMINUTE;
	SECOND_RTC EachSecondFunc = TimeOutput;
	switch (Value){
		case ssHour:
			Flash = HOUR_FLASH,
			ctrlCount = clkadrHOUR;
			break;
		case ssDate:
			Flash = HOUR_FLASH;
			ctrlCount = clkadrDATA;
			EachSecondFunc = ShowDateSet;
			break;
		case ssMonth:
			Flash = MINUT_FLASH;
			ctrlCount = clkadrMONTH;
			EachSecondFunc = ShowDateSet;
			break;
		case ssYear:
			Flash = ALL_FLASH;
			ctrlCount = clkadrYEAR;
			EachSecondFunc = ShowYear;
		default:
			break;
	}
	SettingProcess(EachSecondFunc, Flash);
	SetWrtAdrRTC(ctrlCount);
	SetSecondFunc(EachSecondFunc);
}

/************************************************************************/
/* Установка нового режима и подрежима часов. Устанавливаются так же    */
/* адреса текущих счетчиков и адреса записи в RTC						*/
/************************************************************************/
void SetClockStatus(enum tClockStatus Val, enum tSetStatus SetVal){

	const u08 *txt;

	if (	
		((ClockStatus == csAlarmSet) && (Val != csAlarmSet) && (SetStatus != ssNone))	//Закончена настройка будильников
		||
		((ClockStatus == csSensorSet) && (Val != csSensorSet) && (SetStatus != ssNone))	//Закончена настройка сенсоров
		||
		((ClockStatus == csTune) && (Val != csTune) && (SetStatus != ssNone))			//Или часов. SetStatus != ssNone - что бы прохождение через заголовок настроек не вызывало запись в eeprom
#ifndef IR_SAMSUNG_ONLY
		||
		((ClockStatus == csIRCodeSet) && (Val != csIRCodeSet) && (SetStatus == ssIRCode) && (SetVal != ssIRCode))	//Если происходит смена режима из csIRCodeSet;ssIRCode
#endif
		){
		EeprmStartWrite();								//Записать в eeprom
		if (ClockStatus == csAlarmSet)
			espUartTx(ClkWrite(CLK_ALARM), (u08 *) CurrentShowAlarm, sizeof(struct sAlarm));//Сообщить модулю esp об изменении будильника
		if (ClockStatus == csTune){						//Передать значения в громкости в модуль
			espVolumeTx(vtButton);
			espVolumeTx(vtEachHour);
			espVolumeTx(vtAlarm);
		}
		if (ClockStatus == csSensorSet){				//Сообщить модулю об изменении настройки сенсора
			espSendSensor(CurrentSensorShow);
		}

#ifndef IR_SAMSUNG_ONLY
		if (SetStatus == ssIRCode)						//Это выход из режима настройки пульта ДУ, вернуть обработчик функции кодов пульта на штатный
			IRReciveRdyOn();
#endif
		}

	ClockStatus = Val;
	SetStatus = SetVal;

	switch (Val){
		case csAlarm:									//--------- Будильник сработал. В процедуре ShowTime есть дополнительная проверка режима вывода времени - обычное или мигающее
		case csClock:									//--------- Вывод текущего времени
			CurrentCount = &Watch;						//Текущий счетчик RTC
			if CreepIsOn()								//Если была бегущая строка то очистить дисплей
				ClearDisplay();
			ShowTime();									//Сразу выводится время
			SetSecondFunc(ShowTime);					//Настройка обновления каждую секунду
			Refresh = ShowTime;
			break;
		case csSecond:									//--------- Вывод секунд
			if CreepIsOn()								//Если была бегущая строка то очистить дисплей
				ClearDisplay();
			VertSpeed = VERT_SPEED;
			ShowSecond();
			SetWrtAdrRTC(clkadrSEC);					//Адрес регистра записи в RTC
			SetSecondFunc(ShowSecond);
			Refresh = ShowSecond;
			break;
		case csTempr:									//--------- Вывод температуры
			break;
		case csSet:										//--------- Установка текущего времени
			CurrentCount = &Watch;						//Текущий счетчик RTC
			switch (SetVal){
				case ssNone:
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(setting_word, PHRAZE_START);
					break;
				case ssMinute:
				case ssHour:
				case ssDate:
				case ssMonth:
				case ssYear:
					TimeDateCountSet(SetVal);
					break;
				default:
					while(1);
				break;
			}
			break;
		case csAlarmSet:								//--------- Установка будильника
			switch(SetVal){
				case ssNone:							//Выбор установки из меню
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(alarm_set_word, PHRAZE_START);
					break;
				case ssNumAlarm:						//Выбор номера будильника
					CurrentShowAlarm = FirstAlarm();	//Начальный адрес будильника
					CurrentCount = &(CurrentShowAlarm->Clock);//Текущий счетчик устанавливается в функции SetNextShowAlarm вызываемой из MenuControl при нажатии кнопки ШАГ
					CurrentAlarmDayShow = ALARM_START_DAY;//Текущий день недели для установки будильника - понедельник
				case ssAlarmSw:							//Переключить состояние будильника выключен-включен
					SettingProcess(AlarmNumSetShow, NO_FLASH);
					if (SetVal == ssNumAlarm)
						sputc(S_FLASH_ON, DIGIT2);
					else
						sputc(S_FLASH_ON, DIGIT1);
					break;
				case ssMinute:							//Установка времени и даты будильника
				case ssHour:
				case ssDate:
				case ssMonth:
				case ssYear:
					TimeDateCountSet(SetVal);
					SetSecondFunc(Idle);				//Отключить односекундное обновление
					break;
				case ssAlarmDelay:						//Установка времени звучания будильника
					SettingProcess(AlarmDurationShow, MINUT_FLASH);
					break;
				case ssAlarmTuesd:						//Установка дня недели срабатывания будильника
				case ssAlarmWedn:
				case ssAlarmThur:
				case ssAlarmFrd:
				case ssAlarmSat:
				case ssAlarmSun:
					CurrentAlarmDayShow++;				//Перебираются дни недели за исключением понедельника
				case ssAlarmMondy:						//Установка понедельника в качестве дня срабатывания будильника
					SettingProcess(AlarmNumDayShow, MINUT_FLASH);
					break;
				default:
					break;
			}
			break;
		case csTune:									//--------- Настройка часов
			switch (SetVal){			
				case ssNone:							//Вывод фразы о настройке
				case ssTuneSound:						//Вывод фразы о настройке звука
					SetSecondFunc(Idle);				//Выключить авто обновление
					if (SetVal == ssNone)
						txt = tune_set_word;
					else 
						txt = tune_sound_word;
					WordPut(txt, PHRAZE_START);
					break;
				case ssEvryHour:						//Сигнал каждый час
					ClearDisplay();
					EachHourBeepShow();
					Refresh = EachHourBeepShow;
					break;
				case ssKeyBeep:							//Сигнал на нажатия кнопок
					ClearDisplay();
					KeyBeepShow();
					Refresh = KeyBeepShow;
					break;
				case ssFontPreTune:						//Предварительная фраза о настройке шрифта цифр
					WordPut(tune_font_word, PHRAZE_START);
					Refresh = NULL;
					break;
				case ssFontTune:						//Настройка шрифта цифр
					ClearDisplay();
					WordPut(tune_font_word, PHRAZE_START);
					SettingProcess(FontShow, ALL_FLASH);
					SetSecondFunc(FontShow);
					break;
				case ssSensorTest:						//Нажата кнопка тест на одном из внешних датчиков
					SetSecondFunc(Idle);				
					SensotTestShow();
					Refresh = SensotTestShow;
					break;
#ifdef VOLUME_IS_DIGIT
				case ssVolumeBeep:						//Громкость нажатия кнопок
					ClearDisplay();
					VolumeBeepShow();
					Refresh = VolumeBeepShow;
					break;
				case ssVolumeEachHour:					//Громкость ежечастного сигнала
					ClearDisplay();
					VolumeEachHourShow();
					Refresh = VolumeEachHourShow;
					break;
				case ssVolumeAlarm:						//Громкость будильника
					ClearDisplay();
					VolumeAlarmShow();
					Refresh = VolumeAlarmShow;
					break;
				case ssVolumeTypeAlarm:					//Тип регулировки громкости будильника
					ClearDisplay();
					VolumeAlarmIncShow();
					Refresh = VolumeAlarmIncShow;
					break;
#endif
				case ssHZSpeedTune:						//Настройка скорости горизонтальной прокрутки
				case ssHZSpeedTSet:						//Установка скорости
					SetSecondFunc(Idle);
					WordPut(tune_HZspeed_word, PHRAZE_START);
					if (SetVal == ssHZSpeedTSet)
						ShowDate();
					Refresh = NULL;
					break;					
				case ssTimeSet:							//Запуск получения времени из Интернета
				case ssTimeLoaded:						//Время получено
				case ssTuneNet:							//Предварительная фраза про настройки сети
					SetSecondFunc(Idle);
					if (SetVal == ssTimeSet)
						txt = inet_start_word;
					else if (SetVal == ssTimeLoaded)			
						txt = inet_loaded_word;
					else 
						txt = inet_tune_word;
					WordPut(txt, PHRAZE_START);
					Refresh = NULL;						
					break;
				case ssIP_Show:							//Показать IP часов
					SetSecondFunc(Idle);
					IP_Show();
					Refresh = NULL;
					break;
				default:
					break;
			}
			break;
		case csSensorSet:								//--------- Настройка сенсоров
			switch(SetVal){
				case ssNone:							//Фраза о настройке датчиков
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(sens_set_word, PHRAZE_START);
					break;
				case ssSensNext:						//Выбор датчика
					CurrentSensorShow = 0;				//Начнем с 0 сенсора
				case ssSensSwitch:						//Включение-выключение датчика
					SettingProcess(SensorsNumShow, NO_FLASH);
					if (SetVal == ssSensNext)
						sputc(S_FLASH_ON, DIGIT2);
					else
						sputc(S_FLASH_ON, DIGIT1);
					break;
				case ssSensPreAdr:						//Фраза о выборе адреса датчика
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(sens_set_adr_word, PHRAZE_START);
					break;
				case ssSensAdr:							//Установить адрес для текущего датчика
					SettingProcess(SensorsAdrShow, NO_FLASH);
					sputc(S_FLASH_ON, DIGIT0);
					break;
				case ssSensPreName:						//Сообщение о вводе имени
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(sens_set_name_word, PHRAZE_START);
					sputc(' ', UNDEF_POS);
					sputc(CurrentSensorShow, UNDEF_POS);
					sputc(' ',UNDEF_POS);
					break;
				case ssSensName1:						//Выбор первой буквы имени
					CurrentSensorAlphaNumAndIrCode = 0;
					SettingProcess(SensorsAlphaShow, NO_FLASH);
					sputc(S_FLASH_ON, DIGIT2);
					break;
				case ssSensName2:						//Выбор второй буквы имени
					FlashSet(NO_FLASH);
					sputc(S_FLASH_ON, DIGIT1);
					CurrentSensorAlphaNumAndIrCode++;
					break;
				case ssSensName3:						//Выбор третьей буквы имени
					FlashSet(NO_FLASH);
					sputc(S_FLASH_ON, DIGIT0);
					CurrentSensorAlphaNumAndIrCode++;
					break;
				case ssSensWaite:						//Ожидание датчика. 
					SetSecondFunc(Idle);
					SensorFromIdx(CurrentSensorShow).Value = TMPR_NO_SENS;		//Считаем что температура недействительна
					SensorNoInBus(SensorFromIdx(CurrentSensorShow));			//Датчика на шине нет
					SettingProcess(SensWait, NO_FLASH);
					if (SensorFromIdx(CurrentSensorShow).Adr == SENSOR_DS18B20) //Стартуем сначала измерение потом выводим результат
						StartMeasureDS18();
					else if (SensorFromIdx(CurrentSensorShow).Adr == (EXTERN_TEMP_ADR & SENSOR_ADR_MASK)) //И для датчика на шине I2C
						i2c_ExtTmpr_Read();
					break;
				default:
					break;
			}
			break;
#ifndef IR_SAMSUNG_ONLY
		case csIRCodeSet:								//--------- Настройка клавиш ИК пульта
			switch (SetVal){
				case ssNone:
					SetSecondFunc(Idle);				//Выключить авто обновление
					WordPut(ir_set_word, PHRAZE_START);
					CurrentSensorAlphaNumAndIrCode = 0;//Начать с начала массива кодов
					break;
				case ssIRCode:							//Вывести текущее имя команды пульта ИК
					IRReciveReady = IRRecivCode;		//В режиме записи кода кнопки функция обработки кода пульта другая
					SettingProcess(IRCodeShow, UNDEF_FLASH);
					break;
				default:
					break;
			}
			break;
#endif
		case csSDCardDetect:							//---------- Обнаружена карта в слоте micro-SD, выводится результат инициализации
			ClearDisplay();
			switch (SetVal){
				case ssSDCardNo:						//Не удалось подключить карту
					WordPut(sd_card_err_word, PHRAZE_START);
					break;
				case ssSDCardOk:						//Карта успешно примонтирована
					WordPut(sd_card_ok_word, PHRAZE_START);
					break;
				default:
					break;
			}
			WordPut(sd_card_word, PHRAZE_CONTINUE);
			CreepOn(2);									//Прокрутить текст только 2 раза
			SetSecondFunc(ShowTime);					//По окончании прохода строки вывести время
			Refresh = ShowTime;
			break;
		case csInternetErr:
			SetSecondFunc(Idle);						//Выключить авто обновление
			ClearDisplay();
			CreepInfinteSet();
			Refresh = NULL;								//Автообновление выключить
			break;
		default:
			CurrentCount = &Watch;						//Этот адрес возвращается только для того что бы компилятор не выдавал предупреждения
			CurrentShowAlarm = NULL;
			break;
	}
}

int main(void)
{
	InitRTOS();
	RunRTOS();

	wdt_enable(WDTO_1S);		
//	set_sleep_mode(SLEEP_MODE_PWR_DOWN);						//TODO:не доделано, т.к. не сделан механизм анализа напряжения на выводе наличия питания, возможно и не нужно
	Init_i2c();													//i2C
	Init_i2cRTC();												//RTC. Этот вызов должен быть выполнен как можно скорее после вызова RunRTOS() поскольку в RTOS используется прерывание генерируемое микросхемой RTC
	
	EepromInit();												//Инициализация EEPROM должна выполнятся до инициализации будильников
	DisplayInit();												//Инициализация дисплея может выполнятся в любом месте после разрешения прерываний
	Set0SecundFunc(Check0SecondEachMinut);						//Настроить функцию вызываемую каждую 0-ю секунду в минуте
	SetClockStatus(csClock, ssNone);							//Режим нормального отображения
	AlarmIni();													//Будильники
	SensorIni();												//Инициализация структур внешних датчиков. Должна вызываться после инициализации eeprom

	IRReciverInit();											//Старт работы приемника ИК

	InitKeyboard();												//Клавиатура

	Init_i2cExtTmpr();											//Инициализация внешнего датчика на шине I2C
	StartMeasureDS18();											//Старт измерения температуры на датчике ds18b20
	
	SoundIni();													//генератор ШИМ звука и работа с SD-картой

	KeyBeepSettingSetOff();										//Звук кнопок выключить
	KeyBeepSettingSwitch();
	EachHourSettingSetOff();									//Включить куранты
	EachHourSettingSwitch();

	SetSecondFunc(ShowTime);									//Показывать время каждую секунду

	EeprmReadTune();											//Восстановить настройки часов если они есть
	
	nRF_Init();													//Инициализация модуля nRF24L01+
	espInit();													//Инициализация модуля esp

	while(1){
		Disable_Interrupt;
		if (ClockStatus == csPowerOff){							//Пропало питание. Засыпаем
			wdt_disable();
			DisableIntRTC;										//Что бы RTC не будили
			Enable_Interrupt;									//Что бы появление питания будило
			sleep_enable();
			sleep_cpu();										//Заснули
			sleep_disable();									//Проснулись
			wdt_enable(WDTO_1S);
			EnableIntRTC;
		}
		Enable_Interrupt;
		wdt_reset();
		TaskManager();
	}
}
