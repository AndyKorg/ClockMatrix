/*
 * Построение меню и интерпретация команд с клавиатуры 
 * ver 1.5
 */ 
#include "MenuControl.h"
#include "Clock.h"
#include "i2c.h"
#include "Alarm.h"
#include "Sound.h"
#include "IRrecive.h"
#include "esp8266hal.h"
#ifdef VOLUME_IS_DIGIT
#include "Volume.h"
#endif

//**************** меню *************************************
typedef void (*FUNC_MENU)(void);							//Указатель на действие выполняемое в пункте
typedef u08 (*FUNC_SWITCH)(void);							//Указатель на функцию вызываемую для проверки направления перехода

typedef const PROGMEM struct{								//Пункт меню выполняется при нажатии на кнопку ШАГ
	enum tClockStatus ClockStatus;							//Устанавливаемый режим
	enum tSetStatus SetStatus;								//Устанавливаемый подрежим
	void *BranchMenuOK;										//Указывает на другую ветку меню. Переключается на эту ветку при нажатии ОК. Функция в ActionOK при этом игнорируются
	FUNC_MENU ActionOK;										//Действия выполняемые при нажатии кнопки ОК в этом пункте меню. Исполняется только если BranchMenuOK = NULL
	FUNC_SWITCH Switcher;									//Функция для проверки необходимости перехода, вызывается перед переходом на следующий пункт по команде ШАГ, возвращает 1 или 0
	void *NextItem;											//Следующий пункт меню если Switcher неопределен или возвращает 0
	void *AltenativItem;									//Если Switcher вернет 1 то происходит переход на этот пункт меню по команде ШАГ
} PROGMEM pgmMenuItem;

pgmMenuItem		Null_Menu = {csNone, ssNone, (void *) 0, (void *) 0, (void *) 0, (void *) 0, (void *) 0};//Пустой пнкт меню
pgmMenuItem*	CurrMenuItem;								//Текущий пункт меню

#define NULL_ENTRY	Null_Menu
#define NULL_FUNC	(void*)0

//Чтение структуры меню из flash памяти
#define CLOCK_STATUS (pgm_read_word(&CurrMenuItem->ClockStatus))
#define SET_STATUS (pgm_read_word(&CurrMenuItem->SetStatus))
#define BRANCH_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->BranchMenuOK))
#define EXEC_MENU *((FUNC_MENU*)pgm_read_word(&CurrMenuItem->ActionOK))
#define SWITCH_MENU *((FUNC_SWITCH*)pgm_read_word(&CurrMenuItem->Switcher))
#define NEXT_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->NextItem))
#define ALT_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->AltenativItem))

//Макрос для создания пункта меню в памяти программ
#define MAKE_MENU(Name, ClkStatus, StStatus, Branch, Exec, Swtch, Nxt, Alt) \
	pgmMenuItem	Branch; \
	pgmMenuItem Nxt; \
	pgmMenuItem Alt; \
	pgmMenuItem Name = {ClkStatus, StStatus, (void*)&Branch, (FUNC_MENU)Exec, (FUNC_SWITCH)Swtch, (void *)&Nxt, (void *)&Alt}
		
//Собственно описание пунктов меню
//			Name			ClkStatus,		StStatus,		Branch,			Exec,				Swtch,			Nxt,				Alt
//**************** Главное меню ******************************
MAKE_MENU(mBaseClock,		csClock,		ssNone,			NULL_ENTRY,		StartDateShow,		NULL_FUNC,		mSecondClock,		NULL_ENTRY);	//час и минута
MAKE_MENU(mSecondClock,		csSecond,		ssNone,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mAlarmSetBase,		NULL_ENTRY);	//минуты и секунды
//**************** Выбор будильника **************************
MAKE_MENU(mAlarmSetBase,	csAlarmSet,		ssNone,			mAlarmSetNxt,	NULL_FUNC,			NULL_FUNC,		mClockSet,			NULL_ENTRY);	//уст. будильника
MAKE_MENU(mAlarmSetNxt,		csAlarmSet,		ssNumAlarm,		NULL_ENTRY,		SetNextShowAlarm,	NULL_FUNC,		mAlarmSetSwitch,	NULL_ENTRY);	//номер будильника
MAKE_MENU(mAlarmSetSwitch,	csAlarmSet,		ssAlarmSw,		NULL_ENTRY,		SwitchAlarmStatus,	TestAlarmON,	mBaseClock,			mAlarmSetHour);	//вкл-выкл будильника
//**************** Настройка будильника **********************
MAKE_MENU(mAlarmSetDurat,	csAlarmSet,		ssAlarmDelay,	NULL_ENTRY,		AddDurationAlarm,	NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//----длит. сиг.
MAKE_MENU(mAlarmSetHour,	csAlarmSet,		ssHour,			NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetMinute,	NULL_ENTRY);	//часы будильника
MAKE_MENU(mAlarmSetMinute,	csAlarmSet,		ssMinute,		NULL_ENTRY,		ChangeCounterAlarm,	LitlNumAlarm,	mAlarmSetDate,		mAlarmSetMon);//минуты будил, если номер будильника от 0 до 3 то это еженедельный будильник, иначе будильник по дате
MAKE_MENU(mAlarmSetDate,	csAlarmSet,		ssDate,			NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetMonth,		NULL_ENTRY);	//день будильника
MAKE_MENU(mAlarmSetMonth,	csAlarmSet,		ssMonth,		NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetDurat,		NULL_ENTRY);	//месяц будильника

MAKE_MENU(mAlarmSetMon,		csAlarmSet,		ssAlarmMondy,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetTuesd,		NULL_ENTRY);	//сигн.в понедельник
MAKE_MENU(mAlarmSetTuesd, 	csAlarmSet,		ssAlarmTuesd,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetWeb,		NULL_ENTRY);	//сигн.в вторник
MAKE_MENU(mAlarmSetWeb,		csAlarmSet,		ssAlarmWedn,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetThur,		NULL_ENTRY);	//сигн.в среда
MAKE_MENU(mAlarmSetThur,	csAlarmSet,		ssAlarmThur,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetFri,		NULL_ENTRY);	//сигн.в четверг
MAKE_MENU(mAlarmSetFri,		csAlarmSet,		ssAlarmFrd,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetSat,		NULL_ENTRY);	//сигн.в пятницу
MAKE_MENU(mAlarmSetSat,		csAlarmSet,		ssAlarmSat,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetSun,		NULL_ENTRY);	//сигн.в субботу
MAKE_MENU(mAlarmSetSun,		csAlarmSet,		ssAlarmSun,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetDurat,		NULL_ENTRY);	//сигн.в восркресенье
//**************** Установка времени и даты ******************
MAKE_MENU(mClockSet,		csSet,			ssNone,			mClockSetHour,	NULL_FUNC,			NULL_FUNC,		mClockTune,			NULL_ENTRY);	//установка времени
MAKE_MENU(mClockSetHour,	csSet,			ssHour,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetMinut,		NULL_ENTRY);	//установка минут
MAKE_MENU(mClockSetMinut,	csSet,			ssMinute,		NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetDate,		NULL_ENTRY);	//установка часов
MAKE_MENU(mClockSetDate,	csSet,			ssDate,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetMonth,		NULL_ENTRY);	//   -\\-	даты
MAKE_MENU(mClockSetMonth,	csSet,			ssMonth,		NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetYear,		NULL_ENTRY);	//   -\\-	месяца
MAKE_MENU(mClockSetYear,	csSet,			ssYear,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//   -\\-	года
//**************** Параметры часов ***************************
MAKE_MENU(mClockTune,		csTune,			ssNone,			mClockTuneSound,NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Настройка часов
MAKE_MENU(mClockTuneSound,	csTune,			ssTuneSound,	mClockTuneEach,	NULL_FUNC,			NULL_FUNC,		mClockTuneFont,		NULL_ENTRY);	//Настройка звуков

#ifdef VOLUME_IS_DIGIT
MAKE_MENU(mClockTuneEach,	csTune,			ssEvryHour,		NULL_ENTRY,		EachHourSettingSwitch, EachHourSettingIsSet,mClockTuneBeepKey,	mClockTuneEachVol);	//пищать каждый час
MAKE_MENU(mClockTuneEachVol,csTune,			ssVolumeEachHour,NULL_ENTRY,	VolumeEachHourAdd,	NULL_FUNC,		mClockTuneBeepKey,	NULL_ENTRY);	//громкость писка каждый час
MAKE_MENU(mClockTuneBeepKey,csTune,			ssKeyBeep,		NULL_ENTRY,		KeyBeepSettingSwitch,KeyBeepSettingIsSet,mAlarmVolIncType, mClockTuneBeepKeyVol);	//пищать на кнопки
MAKE_MENU(mClockTuneBeepKeyVol,csTune,		ssVolumeBeep,	NULL_ENTRY,		VolumeKeyBeepAdd,	NULL_FUNC,		mAlarmVolIncType,	NULL_ENTRY);	//громкость звука на кнопках
MAKE_MENU(mAlarmVolIncType,	csTune,			ssVolumeTypeAlarm,NULL_ENTRY,	VolumeTypeTuneAlarm,VolumeAlrmTypeIsNeedLevel,mBaseClock,mClockTuneAlarmVol);	//тип регулирования громкости звука
MAKE_MENU(mClockTuneAlarmVol,csTune,		ssVolumeAlarm,	NULL_ENTRY,		VolumeAlarmAdd,		NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//громкость звука для будильника
#else
MAKE_MENU(mClockTuneEach,	csTune,			ssEvryHour,		NULL_ENTRY,		EachHourSettingSwitch, NULL_FUNC,	mClockTuneBeepKey,	NULL_ENTRY);	//пищать каждый час
MAKE_MENU(mClockTuneBeepKey,csTune,			ssKeyBeep,		NULL_ENTRY,		KeyBeepSettingSwitch, NULL_FUNC,	mBaseClock,			NULL_ENTRY);	//пищать на кнопки
#endif

//**************** Настройка шрифта ***************************
MAKE_MENU(mClockTuneFont,	csTune,			ssFontPreTune,	mClockTuneFontSet,NULL_FUNC,		NULL_FUNC,		mClockTuneHZSpeed,	NULL_ENTRY);	//фраза о настройке шрифта
MAKE_MENU(mClockTuneFontSet,csTune,			ssFontTune,		NULL_ENTRY,		NextFont,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Выбор шрифта

//**************** Скорость бегущей строки ***************************
MAKE_MENU(mClockTuneHZSpeed,csTune,			ssHZSpeedTune,	mClockTuneHZSet,NULL_FUNC,			NULL_FUNC,		mSensTune,			NULL_ENTRY);	//фраза о настройке скорости
MAKE_MENU(mClockTuneHZSet,	csTune,			ssHZSpeedTSet,	NULL_ENTRY,		HorizontalAdd,		NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Выбор скорости

//**************** Работа с esp8266, пункт пропадает если esp не обнаружена на порту uart ***************************
MAKE_MENU(mInternetTune,	csTune,			ssTuneNet,		mInternetIP,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Вывести IP адрес полученный для режима esp station
MAKE_MENU(mInternetIP,		csTune,			ssIP_Show,		NULL_ENTRY,		NULL_FUNC,			NULL_FUNC,		mInternetStart,		NULL_ENTRY);	//Вывести IP адрес полученный для режима esp station
MAKE_MENU(mInternetStart,	csTune,			ssTimeSet,		NULL_ENTRY,		StartGetTimeInternet,NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Запуск получения времени из Интернета

#ifndef IR_SAMSUNG_ONLY						//Настройка для произвольного пульта ДУ
	#define MENU_SENS_END	mIrTune			//Пункт меню на который происходит переход в конце настройки датчиков
	#define MENU_SENS_FUNC	NULL_FUNC
	#define MENU_SENS_ALT	NULL_ENTRY
//**************** Настройка кнопок произвольных пультов ***************************
MAKE_MENU(mIrTune,			csIRCodeSet,	ssNone,			mIrTuneSet,		NULL_FUNC,			espInstalled,	mBaseClock,			mInternetTune);	//Начало настройки кнопок пультов
MAKE_MENU(mIrTuneSet,		csIRCodeSet,	ssIRCode,		NULL_ENTRY,		IRCodeSave,			IRNextCode,		mIrTuneSet,			mBaseClock);	//Определение кода кнопки и занесение его в массив
#else										//Поддержка пультов ДУ только от samsunga
	#define MENU_SENS_END	mBaseClock	//Пункт меню на который происходит переход в конце настройки датчиков
	#define MENU_SENS_FUNC	espInstalled
	#define MENU_SENS_ALT	mInternetTune
#endif

//**************** Настройка датчиков (сенсоров) ***************************
MAKE_MENU(mSensTune,		csSensorSet,	ssNone,			mSensTuneNumSens,NULL_FUNC,			MENU_SENS_FUNC,	MENU_SENS_END,		MENU_SENS_ALT);	//Управление датчиками
MAKE_MENU(mSensTuneNumSens,	csSensorSet,	ssSensNext,		NULL_ENTRY,		SensSetNext,		NULL_FUNC,		mSensSwitch,		NULL_ENTRY);	//Выбор датчика
MAKE_MENU(mSensSwitch,		csSensorSet,	ssSensSwitch,	NULL_ENTRY,		SensSwitch,			SensTestON,		MENU_SENS_END,		mSensPreAdr);	//Включение-выключение датчика
MAKE_MENU(mSensPreAdr,		csSensorSet,	ssSensPreAdr,	mSensAdrSet,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Сообщение о вводе адреса датчика
MAKE_MENU(mSensAdrSet,		csSensorSet,	ssSensAdr,		NULL_ENTRY,		SensAdrSet,			NULL_FUNC,		mSensPreName,		NULL_ENTRY);	//Выбор адреса датчика на шине
MAKE_MENU(mSensPreName,		csSensorSet,	ssSensPreName,	mSensNameSet1,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Сообщение о вводе имени
MAKE_MENU(mSensNameSet1,	csSensorSet,	ssSensName1,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensNameSet2,		NULL_ENTRY);	//Выбор первой буквы имени
MAKE_MENU(mSensNameSet2,	csSensorSet,	ssSensName2,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensNameSet3,		NULL_ENTRY);	//Выбор второй буквы имени
MAKE_MENU(mSensNameSet3,	csSensorSet,	ssSensName3,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensWait,			NULL_ENTRY);	//Выбор третьей буквы имени
MAKE_MENU(mSensWait,		csSensorSet,	ssSensWaite,	mBaseClock,		NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//Ожидание датчика

void MenuIni(void){
	CurrMenuItem = (pgmMenuItem*)&mBaseClock;
}

void MenuStep(void){
	if (((FUNC_SWITCH)&SWITCH_MENU) == NULL_FUNC){							//Нет функции проверки переключения
		CurrMenuItem = &NEXT_MENU;											//Переход на новый пункт меню
	}
	else{
		if (((FUNC_SWITCH)&SWITCH_MENU)() == 1)								//Есть функция проверки, запускаем и по результатам переходим на нужную ветку меню
			CurrMenuItem = &ALT_MENU;
		else
			CurrMenuItem = &NEXT_MENU;
	}
	SetClockStatus(CLOCK_STATUS, SET_STATUS);								//Переключаемся на новый режим
}

void MenuOK(void){
	if (&BRANCH_MENU == &NULL_ENTRY){										//Не требуется переход на следующее меню, только исполнение команды
		if (((FUNC_MENU)&EXEC_MENU) != NULL_FUNC)							//На всякий случай проверяем наличие ссылки на функцию, вдруг ее нету
			((FUNC_MENU)&EXEC_MENU)();										//Выполнить команду
	}
	else {																	//Переход на следующее меню и смена режима
		CurrMenuItem = &BRANCH_MENU;
		SetClockStatus(CLOCK_STATUS, SET_STATUS);
	}
}
