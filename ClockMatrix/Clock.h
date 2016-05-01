/*
 * Clock.h
 * Настройки прошивки под конкретную схему, основные структуры и переменные
 */ 

#ifndef CLOCK_H_
#define CLOCK_H_

#include <avr/io.h>
#include "avrlibtypes.h"

//############################ ОПИСАНИЕ НАСТРОЙКИ ПОД КОНКРЕТНУЮ СХЕМУ  #######################################
#ifndef F_CPU
#define F_CPU 20000000UL							//Если не определена частота в символах проекта определяем тут
#endif

//############################ ДИСПЛЕЙ ########################################################################
//------- Тип внешнего регистра
//#define SHIFT_REG_TYPE_HC595						//Алгоритм для регистров типа 74HC595
#define SHIFT_REG_TYPE_MBI50XX						//Алгоритм для драйверов MBI
//------- Тип индикатора
#define LED_MATRIX_COMMON_COLUMN					//Общие колонки - на трех индикаторах одновременно выводится колонки
//#define LED_MATRIX_COMMON_ROW						//Общие строки - на трех индикаторах одновременно выводится строки

//------- Управление портом SPI дисплея
#define DISP_DDR_SPI		DDRA					//Порт SPI
#define DISP_PORT_SPI		PORTA
#define DISP_SPI_SCK		PORTA1					//Тактовый
#define DISP_SPI_MOSI		PORTA0					//Выход данных
#define DISP_SPI_SS			PORTA6					//Выбор дисплея
#define clckLowLED			ClearBit(DISP_PORT_SPI, DISP_SPI_SCK)
#define clckHightLED		SetBit(DISP_PORT_SPI, DISP_SPI_SCK)	//Запись бита в регистр по перепаду low->hight
#define upldLowLED			ClearBit(DISP_PORT_SPI, DISP_SPI_SS)
#define upldHightLED		SetBit(DISP_PORT_SPI, DISP_SPI_SS)	//Вывод данных на индикатор из регистра по перепаду low->hight
//------- Дополнительное управление дисплеем
#define DISP_OFF_DDR		DDRD					//Порт управления включения-выключения дисплея
#define DISP_OFF_PORT		PORTD
#define DISP_OFF_PIN		PORTD7
#define DisplayOff			SetBit(DISP_OFF_PORT, DISP_OFF_PIN)
#define DisplayOn			ClearBit(DISP_OFF_PORT, DISP_OFF_PIN)
#ifdef SHIFT_REG_TYPE_HC595
	#define DISP_CLR_DDR	DDRA					//Порт очистки регистра дисплея
	#define DISP_CLR_PORT	PORTA
	#define DISP_CLR_PIN	PORTA4
	#define DisplayClrReg	ClearBit(DISP_CLR_PORT, DISP_CLR_PIN)
	#define DisplayNoClrReg	SetBit(DISP_CLR_PORT, DISP_CLR_PIN)
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	#define DISP_SDO_DDR	DDRA					//Вход на который подается диагностика от MBI5039
	#define DISP_SDO_PORT	PORTA
	#define DISP_SDO_PIN	PORTA4
#else
	#error "Shift register undefined"
#endif
//------- Регулировка яркости в зависимости от освещенности
#ifdef LED_MATRIX_COMMON_COLUMN
	#define DISP_STEP_OUT	3						//Количество проходов в прерывании 32 кГц для полной подготовки регистров вывода на дисплей
#elif defined(LED_MATRIX_COMMON_ROW)
	#define DISP_STEP_OUT	12						//Количество проходов в прерывании 32 кГц для полной подготовки регистров вывода на дисплей
#else
	#error "Undefined display type"
#endif
#define DELAY_DISP			64						//Задержка между выводом столбцов. Не должна быть большой т.к. светодиоды начинают мерцать.
#define DISP_BOUNDARY_LVL	12						//Граница освещенности до которой регулировка яркости производится по одной ступени
#define DELAY_INTERVAL		2						//Скорость приращения яркости после пересечения значения DISP_BOUNDARY_LVL
#define ILLUM_PORT		(0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0) //Аналоговый порт измерителя освещенности, сейчас ADC2
#define PrescalerADC	(1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) //Прескаллер АЦП на как можно большее деление частоты, т.к. скорость преобразования не важна
//------- Структура дисплея
#define DISP_ROW			8						//Количество строк в дисплее
#define DISP_COL_DOT		8						//Количество точек в столбце
#define DISP_COL			24						//Количество столбцов в дисплее
#define DISP_DIGIT			(DISP_COL/DISP_COL_DOT)	//Количество восмибитных разрядов


//############################ РЕГУЛИРОВКА ГРОМКОСТИ  ########################################################################
#define VOLUME_IS_DIGIT								//Закоментировать строку если не используется цифровой регулятор громкости

//############################ ИНТЕРФЕЙС SPI for SD-CARD  ####################################################################
//---------  Программный или аппаратный
#define SD_SPI_BUILD_IN								//Закоментировать если используется программный SPI, а не аппаратный
//---------  Тип преобразователя уровней 5В->3.3В
//#define SD_SPI_DIRECT								//Закоментировать если используется инвертированные буферы при преобразовании 5В в 3.3В
//---------  Порты SPI
#define SD_PORT_DDR			DDRB					//Порт для интерфейса SPI
#define SD_PORT_OUT			PORTB
#define SD_PORT_IN			PINB
#define SD_PORT_CS_DDR		DDRB					//Порт для вывода CS
#define SD_PORT_CS_OUT		PORTB
#define SD_PORT_CS_IN		PINB
//---------  Pins SPI
#define SD_CS				PORTB4					//Для аппаратного SPI лучше выбирать пин отличный от SS аппаратного, т.к. происходит конфликт при программировании через SPI
#ifdef SD_SPI_BUILD_IN
#define SD_DI				PORTB5					//Аппаратный SPI
#define SD_DO				PORTB6
#define SD_CLK				PORTB7
#else
#define SD_DI				PORTB5					//Программный SPI
#define SD_DO				PORTB6
#define SD_CLK				PORTB7
#endif
//---------  Датчики SD-card
#define SD_PORT_CASEDDR		DDRD					//Порт для датчика наличия карточки и запрета записи
#define SD_PORT_CASEIN		PIND
#define SD_INS				PORTD5					//Наличие карточки в слоту
#define SD_WP				PORTB1					//Датчик защиты записи - в часах не используется, оставлен для совместимости
#define SD_TEST_PERIOD		100						//Период проверки наличия SD-карты в слоту, мс
#define SD_WHITE_PERIOD		300						//Ожидание перед инициализацией карточки
#define NoSDCard()			((SD_PORT_CASEIN & (1<<SD_INS)) != 0x00)	//Нет карточки в слоту
#define SD_CARD_ATTEMPT_MAX	10						//Максимальное количество попыток примонтировать сд-карту

//############################ КЛАВИАТУРА  ########################################################################
//#define EXTERNAL_INTERAPTS_USE					//Использовать внешние прерывания для опроса кнопок

//------------- Описание кнопок
#ifdef EXTERNAL_INTERAPTS_USE
	#define KEY_STEP_INT		INT0				//Бит маски прерывания кнопки Step
	#define KEY_STEP_INTname	INT0_vect			//Имя прерывания для STEP
	#define KEY_OK_INT			INT1				//кнопки ОК
	#define KEY_OK_INTname		INT1_vect			//Имя прерывания
	#define KEY_INT_MASK		(Bit(KEY_STEP_INT) | Bit(KEY_OK_INT))
#else
	#define KEY_PORTIN_STEP		PIND				//Порт кнопки STEP
	#define	KEY_PORTIN_OK		PINA				//Порт кнопки OK
#endif

#define KEY_STEP_PORT			PIND4				//Линии ввода кнопок
#define KEY_OK_PORT				PINA5

#define KEY_STEP_FLAG			0					//Бит кнопки STEP в переменных состояния кнопок
#define KEY_OK_FLAG				1					//Бит кнопки ОК

#ifndef EXTERNAL_INTERAPTS_USE
	#define SCAN_PERIOD_KEY		150					//Период сканирования кнопок если не используются прерывания
#endif

#define PROTECT_PRD_KEY			30					//Защитный интервал для подавления дребезга контактов, мс

//############################ ЦИФРОВОЙ РЕГУЛЯТОР ГРОМКОСТИ ########################################################################
#define VOLUME_DDR_CS_SHDWM	DDRB					//Порт для работы с регулятором для выводов CS и SHDWM
#define VOLUME_PRT_CS_SHDWM	PORTB
#define VOLUME_SHDWN		PORTB1					//Порт вывода выключения источника сигнала
#define VOLUME_CS			PORTB0					//Порт вывода CS
#define VOLUME_DDR_CLK_SI	DDRC					//Порт для работы с регулятором для выводов CLK и SI
#define VOLUME_PRT_CLK_SI	PORTC
#define VOLUME_CLK			PORTC7					//Порт вывода CLK
#define VOLUME_SI			PORTC6					//Порт вывода данных SI

//############################ ВНУТРЕННИЕ ТАЙМЕРЫ ########################################################################
// Таймер 0 - используется генератором ШИМ звука настроен в Sound.c, выведен на OC0
// Таймер 1 - используется генератором ШИМ звука настроен в Sound.c, отмеряет битрейт
// Таймер 2 - используется для отсчета интервалов на шине 1-ware, настроен в OneWare.c

//############################ ВНЕШНИЕ ПРЕРЫВАНИЯ ########################################################################
#define IR_INT				INT0					//Прерывание на котором висит ИК приемник
#define DISP_INT_VECTOR_32HZ INT1_vect				//Вектор прерывания от генератора 32768 Гц
// INT2 - Используется для отсчета односекундных интервалов (имя INT_NAME_RTC), описан в i2c.h
//------------- Ежесекундное прерывание от RTC от отдельной ноги. Закоментировать строчку ниже если используется прерывание PCINT и оставить если используется INTX
#define INT_SENCE_RTC		Bit(ISC2)				//Срабатывание по нарастающему фронту
#ifdef INT_SENCE_RTC
	// --- Для работы с прерыванием INT
	#define INT_NAME_RTC	INT2_vect				//Имя прерывания от RTC
	#define INT_RTCreg		GICR					//Регистр маски прерывания
	#define	INT_RTC			INT2					//Маска прерывания
#else
	// --- Для работы с прерыванием PCINT
	#define INT_RTCgrp		PCIE2					//Группа прерываний от RTC
	#define INT_NAME_RTC	PCINT2_vect				//Имя прерывания от RTC
	#define INT_RTCreg		PCMSK2					//Регистр маски прерывания
	#define	INT_RTC			PCINT18					//Маска прерывания
#endif

//############################ ИНТЕРФЕЙС К ШИНЕ I2C описан в IIC_ultimate.h  ########################################################################

//############################ ИК-приемник  #########################################################################################################
//#define IR_SAMSUNG_ONLY					//Закоментировать если используется произвольный пульт. В этом случае ИК канал для внешнего датчика будет недоступен
#if (IR_INT == INT0)
	#define IR_INT_BIT0		ISC00
	#define IR_INT_BIT1		ISC01
	#define IR_INT_VECT		INT0_vect
#elif (IR_INT == INT1)
	#define IR_INT_BIT0		ISC10
	#define IR_INT_BIT1		ISC11
	#define IR_INT_VECT		INT1_vect
#else
	#error "IR interrupt no defined!"
#endif

#define IntiRDownFront()	do {SetBit(MCUCR, IR_INT_BIT1); ClearBit(MCUCR, IR_INT_BIT0);} while (0)	//Настроить прерывание IR на спадающий фронт
#define IntIRUpFront()		do {SetBit(MCUCR, IR_INT_BIT1); SetBit(MCUCR, IR_INT_BIT0);} while (0)		//нарастающий фронт
#define IntIRUpDownFront()	do {ClearBit(MCUCR, IR_INT_BIT1); SetBit(MCUCR, IR_INT_BIT0);} while (0)	//И нарастающий фронт и спадающий вызывает прерывание
#define StartIRCounting()	SetBit(GICR, IR_INT)	//Начать работу
#define StopIRCounting()	ClearBit(GICR, IR_INT)	//Остановить прием импульсов

//############################ Контроль питания - ПОКА НЕ ИСПОЛЬЗУЕТСЯ ########################################################################
// TODO: доделать контроль питания
/*
#define POWER_ENBL_INT	PCINT8						//Бит прерывания пропадания напряжения питания

#define POWER_INTgrp	PCIE1						//Группа прерываний для контроля напряжения питания сети
#define POWER_INTname	PCINT1_vect					//Имя прерывания -\\- КОд прерывания находится в keyboard.c
#define POWER_INTreg	PCMSK1						//Имя регистра для маски прерывания -\\-
#define POWER_PORTin	PINC						//Порт подключения
*/


//----------------- КОНЕЦ ОПИСАНИЯ НАСТРОЙКИ ПОД КОНКРЕТНУЮ СХЕМУ ---------------------------------------------------------------

//############################ РАЗЛИЧНЫЕ НАСТРОЙКИ ЧАСОВ ########################################################################

//---------  Сигналы каждый час (куранты)
#define EACH_HOUR_START		9						//Час с которого начинается бой курантов если он разрешен
#define EACH_HOUR_STOP		21						//Час до которого бой продолжается если он разрешен

//---------  Работа с интернетом
#define INET_TIME_DAY		6						//Суббота - Номер дня недели в который будет запрошено точное время из Инета
#define INET_TIME_HOUR		0x10					//10 часов утра - Час для запроса точного времени. В формате BCD в 24-х часовом формате TODO: доделать для поддержки 12-и часового формата
#define INET_TiME_MINUTE	0x10					//10 минут Минута для запроса точного времени. В формате BCD

//---------  Прочие
#define SECOND_DAY_SHOW		15						//На какой секунде выводить бегущую строку с датой в режиме вывода текущего времени

#define CENTURY_DEF			0x20					//Номер века

#define TIMEOUT_RET_CLOCK_MODE_MIN	10				//Таймаут возврата в основной режим работы часов, в минутах, не действует в некоторых режимах.


//############################ КОНЕЦ РАЗЛИЧНЫХ НАСТРОЕК ЧАСОВ ########################################################################


//############################ ОПРЕДЕЛЕНИЯ ###########################################################################################
typedef void (*VOID_PTR_VOID)(void);				//Указатель на функцию, для косвенного вызова функции

//---------  Управление статусами часов и будильников
enum tClockStatus									//Режимы часов
{
	csNone,											//Пустой режим
	csClock,										//Отображается текущее время
	csSecond,										//Только секунды
	csDate,											//Отображается дата, месяц, день неделя
	csSet,											//установка времени
	csAlarmSet,										//установка будильников
	csAlarm,										//Будильник сработал
	csTune,											//Настройка параметров часов
	csTempr,										//Вывод температуры
	csSensorSet,									//Настройка сенсоров
	csSDCardDetect,									//Обнаружено появление SD-карты в слоту, считается режимом установки
#ifndef IR_SAMSUNG_ONLY
	csIRCodeSet,									//Настройка кодов ИК-пульта
#endif	
	csInternetErr,									//Режим вывода ошибки при обращении к esp8266
	csPowerOff										//Выключение в связи пропаданием сетевого питания
};

enum tSetStatus{									//Подрежим режима установки
	ssNone,											//Нормальный режим или поясняющий пункт меню
	ssSecond,										//Установка секунд - следующее нажатие на UP сбрасывает секунды в ноль
	ssMinute,										//Установка минут
	ssHour,											//часов
	ssDate,											//Даты. Для режима установки будильников не доступны этот и все ниже следующие подрежимы
	ssMonth,										//месяц
	ssYear,											//год
	ssNumAlarm,										//Номер будильника
	ssAlarmSw,										//Включение-выключение будильника
	ssAlarmDelay,									//Длительность будильника
	ssAlarmMondy,									//Сигнал будильника в понедельник
	ssAlarmTuesd,									//Сигнал будильника в вторник
	ssAlarmWedn,									//Сигнал будильника в среду
	ssAlarmThur,									//Сигнал будильника в четверг
	ssAlarmFrd,										//Сигнал будильника в пятницу
	ssAlarmSat,										//Сигнал будильника в субботу
	ssAlarmSun,										//Сигнал будильника в воскресенье
	ssTuneSound,									//Поясняющий пункт перед входом в меню управления доступом
	ssEvryHour,										//Пищать каждый час
	ssKeyBeep,										//Пищать на нажатие кнопок
#ifdef VOLUME_IS_DIGIT
	ssVolumeBeep,									//Громкость звука нажатия кнопок
	ssVolumeEachHour,								// -/- ежечасного сигнала
	ssVolumeAlarm,									// -/- будильника
	ssVolumeTypeAlarm,								//Тип регулировки громкости
#endif
	ssFontPreTune,									//Предварительная фраза для настройки шрифта
	ssFontTune,										//Настроить шрифт
	ssHZSpeedTune,									//Скорость горизонтальной прокрутки
	ssHZSpeedTSet,									//Установка скорости
	ssSensNext,										//Выбор датчика
	ssSensSwitch,									//Включение-выключение датчика
	ssSensPreAdr,									//Сообщение о выборе адреса датчика
	ssSensAdr,										//Выбор адреса датчика
	ssSensPreName,									//Сообщение о вводе имени
	ssSensName1,									//Выбор первой буквы имени
	ssSensName2,									//Выбор второй буквы имени
	ssSensName3,									//Выбор третьей буквы имени
	ssSensWaite,									//Ожидание подключения датчика
#ifndef IR_SAMSUNG_ONLY								//Если используется произвольный ИК пульт
	ssIRCode,										//Перебираются команды для кнопок пульта ИК
#endif
	ssSDCardNo,										//SD-карта отсутствует
	ssSDCardOk,										//SD-карта обнаружена и инициализирована
	ssTimeSet,										//Запуск и работа запроса из Интернета
	ssTimeLoaded,									//Время установлено
	ssTuneNet,										//Настройки сети
	ssIP_Show										//Вывод адреса IP для режима station
};

struct sClockValue{									//Значения времени для часов и будильников. В BCD-формате.
	u08	Second;										//Текущее время обновляется с частотой REFRESH_CLOCK.
	u08	Minute;
	u08	Hour;
	//u08 Day;										//День недели в импортном формате, 1 - воскресенье, 2 - понедельник и т.д.
	u08 Date;										//Дата месяца
	u08 Month;									
	u08	Year;										//От 0 до 99, всегда 21 век.
	u08 Mode;										//Режим работы - см. ниже
};

extern volatile struct sClockValue Watch;			//Текущее значение часов. Поскольку член Watch.Mode всегда взведен то он используется как флаговая переменная в различных подсистемах

//----------------- Биты переменной Mode в структуре sClockValue.
//---------  Состояние автомата записи в eeprom. В качестве флаговой переменной используется именно Watch.Mode
#define epSTATE_BIT			7						//0-автомат свободен, 1 - занят
#define epWriteIsBusy()		BitIsSet(Watch.Mode, epSTATE_BIT)	//Автомат записи занят
#define epWriteIsFree()		BitIsClear(Watch.Mode, epSTATE_BIT)	//Автомат свободен
#define epWriteBusy()		SetBit(Watch.Mode, epSTATE_BIT)		//Занять автомат записи
#define epWriteFree()		ClearBit(Watch.Mode, epSTATE_BIT)	//Освободить автомат
//---------  Режимы отсчета количества часов.
#define mcModeHourBit		6						//Номер бита режима отсчета количества часов, ВНИМАНИЕ! совпадает с номером бита в RTC!
#define mcModeHourMask		Bit(mcModeHourBit)		//Маска флага режима по полудни или до полудня
#define SetMode12(Mc)		SetBit(Mc, mcModeHourBit)//Установить режим 12-и часовой
#define SetMode24(Mc)		ClearBit(Mc, mcModeHourBit)	//режим 24 часа
#define ModeIs12(Mc)		BitIsSet(Mc, mcModeHourBit)
#define ModeIs24(Mc)		BitIsClear(Mc, mcModeHourBit)

#define mcAM_PM_Bit			5						//Номер бита AM/PM, ВНИМАНИЕ! совпадает с номером бита в RTC!
#define mcAM_PM_Mask		Bit(mcAM_PM_Bit)		//Маска флага режима по полудни или до полудня
#define SetAM(Mc)			ClearBit(Mc, mcAM_PM_Bit)//Установить время AM
#define SetPM(Mc)			SetBit(Mc, mcAM_PM_Bit)	//Установить время PM
#define ClockIsAM(Mc)		BitIsClear(Mc, mcAM_PM_Bit)
#define ClockIsPM(Mc)		BitIsSet(Mc, mcAM_PM_Bit)
#define ClrModeBit(mc)		(mc & ~(mcModeHourMask | mcAM_PM_Mask))	//Сброс режимов часов и AM/PM
//---------  Флаг подключения модуля esp8266
#define espInstallBit		4						//Модуль esp8266 установлен
#define espModuleSet()		SetBit(Watch.Mode, espInstallBit)
#define espModuleRemove()	ClearBit(Watch.Mode, espInstallBit)
#define espModuleIsSet()	BitIsSet(Watch.Mode, espInstallBit)
#define	espModuleIsNot()	BitIsClear(Watch.Mode, espInstallBit)
//---------  Состояние RTC после старта МК
#define RTC_NotValidBit		3						//Было отключение генератора RTC
#define RTC_IsValid()		BitIsClear(Watch.Mode, RTC_NotValidBit)	//RTC корректна
#define RTC_IsNotValid()	BitIsSet(Watch.Mode, RTC_NotValidBit)	//Был сбой питания и RTC некорректна
#define RTC_ValidSet()		ClearBit(Watch.Mode, RTC_NotValidBit)	//Установить флаг корректности RTC
#define RTC_ValidClear()	SetBit(Watch.Mode, RTC_NotValidBit)		//Сбросить флаг корректности RTC
//Свободные биты
//2
//1
//0

//---------  Управление датчиками в меню
void SensSetNext(void);								//Выбор следующего датчика для отображения
void SensAdrSet(void);								//Выбор адреса датчика
void SensNameSet(void);								//Переключение буквы в имени датчика
void SensSwitch(void);								//Выключение-выключение текущего датчика
u08 SensTestON(void);								//Проверка состояния датчика.

//---------  Настройка кодов кнопок произвольного пульта ДУ
#ifndef IR_SAMSUNG_ONLY
void IRCodeSave(void);								//Записать код клавиши
u08 IRNextCode(void);								//Следующий индекс в массиве кодов пульта ДУ. Возвращает 1 если все команды были перебраны
#endif

//############################ Общечасовые переменные и функции ###########################################################################################
#define MULTIx8(c) ((c)<<3)							//Замена операции умножения на 8. Осторожнее с переполнением!

extern volatile struct sClockValue *CurrentCount;	//Текущий счетчик часов. Может быть и обычными часами, и часами будильника
extern struct sAlarm *CurrentShowAlarm;				//Текущий отображаемый будильник
extern u08	CurrentAlarmDayShow,					//Текущий день недели для текущего будильника
			CurrentSensorShow;						//Текущий настраиваемый датчик

extern volatile enum tClockStatus ClockStatus;		//Состояние часов
extern volatile enum tSetStatus SetStatus;			//Подрежим в режиме установки

void SetClockStatus(volatile enum tClockStatus Val, enum tSetStatus SetVal);//Установка нового режима часов

void StartDateShow(void);							//Показать дату один раз
void ShowDate(void);								//Сформировать строку даты и инфы с датчиков

VOID_PTR_VOID Refresh;								//Ссылка на функцию обновляющую буфер дисплея после расчетов
	
#endif /* CLOCK_H_ */