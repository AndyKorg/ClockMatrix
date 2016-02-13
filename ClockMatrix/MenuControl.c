/*
 * ���������� ���� � ������������� ������ � ���������� 
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

//**************** ���� *************************************
typedef void (*FUNC_MENU)(void);							//��������� �� �������� ����������� � ������
typedef u08 (*FUNC_SWITCH)(void);							//��������� �� ������� ���������� ��� �������� ����������� ��������

typedef const PROGMEM struct{								//����� ���� ����������� ��� ������� �� ������ ���
	enum tClockStatus ClockStatus;							//��������������� �����
	enum tSetStatus SetStatus;								//��������������� ��������
	void *BranchMenuOK;										//��������� �� ������ ����� ����. ������������� �� ��� ����� ��� ������� ��. ������� � ActionOK ��� ���� ������������
	FUNC_MENU ActionOK;										//�������� ����������� ��� ������� ������ �� � ���� ������ ����. ����������� ������ ���� BranchMenuOK = NULL
	FUNC_SWITCH Switcher;									//������� ��� �������� ������������� ��������, ���������� ����� ��������� �� ��������� ����� �� ������� ���, ���������� 1 ��� 0
	void *NextItem;											//��������� ����� ���� ���� Switcher ����������� ��� ���������� 0
	void *AltenativItem;									//���� Switcher ������ 1 �� ���������� ������� �� ���� ����� ���� �� ������� ���
} PROGMEM pgmMenuItem;

pgmMenuItem		Null_Menu = {csNone, ssNone, (void *) 0, (void *) 0, (void *) 0, (void *) 0, (void *) 0};//������ ���� ����
pgmMenuItem*	CurrMenuItem;								//������� ����� ����

#define NULL_ENTRY	Null_Menu
#define NULL_FUNC	(void*)0

//������ ��������� ���� �� flash ������
#define CLOCK_STATUS (pgm_read_word(&CurrMenuItem->ClockStatus))
#define SET_STATUS (pgm_read_word(&CurrMenuItem->SetStatus))
#define BRANCH_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->BranchMenuOK))
#define EXEC_MENU *((FUNC_MENU*)pgm_read_word(&CurrMenuItem->ActionOK))
#define SWITCH_MENU *((FUNC_SWITCH*)pgm_read_word(&CurrMenuItem->Switcher))
#define NEXT_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->NextItem))
#define ALT_MENU *((pgmMenuItem*)pgm_read_word(&CurrMenuItem->AltenativItem))

//������ ��� �������� ������ ���� � ������ ��������
#define MAKE_MENU(Name, ClkStatus, StStatus, Branch, Exec, Swtch, Nxt, Alt) \
	pgmMenuItem	Branch; \
	pgmMenuItem Nxt; \
	pgmMenuItem Alt; \
	pgmMenuItem Name = {ClkStatus, StStatus, (void*)&Branch, (FUNC_MENU)Exec, (FUNC_SWITCH)Swtch, (void *)&Nxt, (void *)&Alt}
		
//���������� �������� ������� ����
//			Name			ClkStatus,		StStatus,		Branch,			Exec,				Swtch,			Nxt,				Alt
//**************** ������� ���� ******************************
MAKE_MENU(mBaseClock,		csClock,		ssNone,			NULL_ENTRY,		StartDateShow,		NULL_FUNC,		mSecondClock,		NULL_ENTRY);	//��� � ������
MAKE_MENU(mSecondClock,		csSecond,		ssNone,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mAlarmSetBase,		NULL_ENTRY);	//������ � �������
//**************** ����� ���������� **************************
MAKE_MENU(mAlarmSetBase,	csAlarmSet,		ssNone,			mAlarmSetNxt,	NULL_FUNC,			NULL_FUNC,		mClockSet,			NULL_ENTRY);	//���. ����������
MAKE_MENU(mAlarmSetNxt,		csAlarmSet,		ssNumAlarm,		NULL_ENTRY,		SetNextShowAlarm,	NULL_FUNC,		mAlarmSetSwitch,	NULL_ENTRY);	//����� ����������
MAKE_MENU(mAlarmSetSwitch,	csAlarmSet,		ssAlarmSw,		NULL_ENTRY,		SwitchAlarmStatus,	TestAlarmON,	mBaseClock,			mAlarmSetHour);	//���-���� ����������
//**************** ��������� ���������� **********************
MAKE_MENU(mAlarmSetDurat,	csAlarmSet,		ssAlarmDelay,	NULL_ENTRY,		AddDurationAlarm,	NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//----����. ���.
MAKE_MENU(mAlarmSetHour,	csAlarmSet,		ssHour,			NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetMinute,	NULL_ENTRY);	//���� ����������
MAKE_MENU(mAlarmSetMinute,	csAlarmSet,		ssMinute,		NULL_ENTRY,		ChangeCounterAlarm,	LitlNumAlarm,	mAlarmSetDate,		mAlarmSetMon);//������ �����, ���� ����� ���������� �� 0 �� 3 �� ��� ������������ ���������, ����� ��������� �� ����
MAKE_MENU(mAlarmSetDate,	csAlarmSet,		ssDate,			NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetMonth,		NULL_ENTRY);	//���� ����������
MAKE_MENU(mAlarmSetMonth,	csAlarmSet,		ssMonth,		NULL_ENTRY,		ChangeCounterAlarm,	NULL_FUNC,		mAlarmSetDurat,		NULL_ENTRY);	//����� ����������

MAKE_MENU(mAlarmSetMon,		csAlarmSet,		ssAlarmMondy,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetTuesd,		NULL_ENTRY);	//����.� �����������
MAKE_MENU(mAlarmSetTuesd, 	csAlarmSet,		ssAlarmTuesd,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetWeb,		NULL_ENTRY);	//����.� �������
MAKE_MENU(mAlarmSetWeb,		csAlarmSet,		ssAlarmWedn,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetThur,		NULL_ENTRY);	//����.� �����
MAKE_MENU(mAlarmSetThur,	csAlarmSet,		ssAlarmThur,	NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetFri,		NULL_ENTRY);	//����.� �������
MAKE_MENU(mAlarmSetFri,		csAlarmSet,		ssAlarmFrd,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetSat,		NULL_ENTRY);	//����.� �������
MAKE_MENU(mAlarmSetSat,		csAlarmSet,		ssAlarmSat,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetSun,		NULL_ENTRY);	//����.� �������
MAKE_MENU(mAlarmSetSun,		csAlarmSet,		ssAlarmSun,		NULL_ENTRY,		AlarmDaySwitch,		NULL_FUNC,		mAlarmSetDurat,		NULL_ENTRY);	//����.� ������������
//**************** ��������� ������� � ���� ******************
MAKE_MENU(mClockSet,		csSet,			ssNone,			mClockSetHour,	NULL_FUNC,			NULL_FUNC,		mClockTune,			NULL_ENTRY);	//��������� �������
MAKE_MENU(mClockSetHour,	csSet,			ssHour,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetMinut,		NULL_ENTRY);	//��������� �����
MAKE_MENU(mClockSetMinut,	csSet,			ssMinute,		NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetDate,		NULL_ENTRY);	//��������� �����
MAKE_MENU(mClockSetDate,	csSet,			ssDate,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetMonth,		NULL_ENTRY);	//   -\\-	����
MAKE_MENU(mClockSetMonth,	csSet,			ssMonth,		NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mClockSetYear,		NULL_ENTRY);	//   -\\-	������
MAKE_MENU(mClockSetYear,	csSet,			ssYear,			NULL_ENTRY,		WriteToRTC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//   -\\-	����
//**************** ��������� ����� ***************************
MAKE_MENU(mClockTune,		csTune,			ssNone,			mClockTuneSound,NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//��������� �����
MAKE_MENU(mClockTuneSound,	csTune,			ssTuneSound,	mClockTuneEach,	NULL_FUNC,			NULL_FUNC,		mClockTuneFont,		NULL_ENTRY);	//��������� ������

#ifdef VOLUME_IS_DIGIT
MAKE_MENU(mClockTuneEach,	csTune,			ssEvryHour,		NULL_ENTRY,		EachHourSettingSwitch, EachHourSettingIsSet,mClockTuneBeepKey,	mClockTuneEachVol);	//������ ������ ���
MAKE_MENU(mClockTuneEachVol,csTune,			ssVolumeEachHour,NULL_ENTRY,	VolumeEachHourAdd,	NULL_FUNC,		mClockTuneBeepKey,	NULL_ENTRY);	//��������� ����� ������ ���
MAKE_MENU(mClockTuneBeepKey,csTune,			ssKeyBeep,		NULL_ENTRY,		KeyBeepSettingSwitch,KeyBeepSettingIsSet,mAlarmVolIncType, mClockTuneBeepKeyVol);	//������ �� ������
MAKE_MENU(mClockTuneBeepKeyVol,csTune,		ssVolumeBeep,	NULL_ENTRY,		VolumeKeyBeepAdd,	NULL_FUNC,		mAlarmVolIncType,	NULL_ENTRY);	//��������� ����� �� �������
MAKE_MENU(mAlarmVolIncType,	csTune,			ssVolumeTypeAlarm,NULL_ENTRY,	VolumeTypeTuneAlarm,VolumeAlrmTypeIsNeedLevel,mBaseClock,mClockTuneAlarmVol);	//��� ������������� ��������� �����
MAKE_MENU(mClockTuneAlarmVol,csTune,		ssVolumeAlarm,	NULL_ENTRY,		VolumeAlarmAdd,		NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//��������� ����� ��� ����������
#else
MAKE_MENU(mClockTuneEach,	csTune,			ssEvryHour,		NULL_ENTRY,		EachHourSettingSwitch, NULL_FUNC,	mClockTuneBeepKey,	NULL_ENTRY);	//������ ������ ���
MAKE_MENU(mClockTuneBeepKey,csTune,			ssKeyBeep,		NULL_ENTRY,		KeyBeepSettingSwitch, NULL_FUNC,	mBaseClock,			NULL_ENTRY);	//������ �� ������
#endif

//**************** ��������� ������ ***************************
MAKE_MENU(mClockTuneFont,	csTune,			ssFontPreTune,	mClockTuneFontSet,NULL_FUNC,		NULL_FUNC,		mClockTuneHZSpeed,	NULL_ENTRY);	//����� � ��������� ������
MAKE_MENU(mClockTuneFontSet,csTune,			ssFontTune,		NULL_ENTRY,		NextFont,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//����� ������

//**************** �������� ������� ������ ***************************
MAKE_MENU(mClockTuneHZSpeed,csTune,			ssHZSpeedTune,	mClockTuneHZSet,NULL_FUNC,			NULL_FUNC,		mSensTune,			NULL_ENTRY);	//����� � ��������� ��������
MAKE_MENU(mClockTuneHZSet,	csTune,			ssHZSpeedTSet,	NULL_ENTRY,		HorizontalAdd,		NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//����� ��������

//**************** ������ � esp8266, ����� ��������� ���� esp �� ���������� �� ����� uart ***************************
MAKE_MENU(mInternetTune,	csTune,			ssTuneNet,		mInternetIP,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//������� IP ����� ���������� ��� ������ esp station
MAKE_MENU(mInternetIP,		csTune,			ssIP_Show,		NULL_ENTRY,		NULL_FUNC,			NULL_FUNC,		mInternetStart,		NULL_ENTRY);	//������� IP ����� ���������� ��� ������ esp station
MAKE_MENU(mInternetStart,	csTune,			ssTimeSet,		NULL_ENTRY,		StartGetTimeInternet,NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//������ ��������� ������� �� ���������

#ifndef IR_SAMSUNG_ONLY						//��������� ��� ������������� ������ ��
	#define MENU_SENS_END	mIrTune			//����� ���� �� ������� ���������� ������� � ����� ��������� ��������
	#define MENU_SENS_FUNC	NULL_FUNC
	#define MENU_SENS_ALT	NULL_ENTRY
//**************** ��������� ������ ������������ ������� ***************************
MAKE_MENU(mIrTune,			csIRCodeSet,	ssNone,			mIrTuneSet,		NULL_FUNC,			espInstalled,	mBaseClock,			mInternetTune);	//������ ��������� ������ �������
MAKE_MENU(mIrTuneSet,		csIRCodeSet,	ssIRCode,		NULL_ENTRY,		IRCodeSave,			IRNextCode,		mIrTuneSet,			mBaseClock);	//����������� ���� ������ � ��������� ��� � ������
#else										//��������� ������� �� ������ �� samsunga
	#define MENU_SENS_END	mBaseClock	//����� ���� �� ������� ���������� ������� � ����� ��������� ��������
	#define MENU_SENS_FUNC	espInstalled
	#define MENU_SENS_ALT	mInternetTune
#endif

//**************** ��������� �������� (��������) ***************************
MAKE_MENU(mSensTune,		csSensorSet,	ssNone,			mSensTuneNumSens,NULL_FUNC,			MENU_SENS_FUNC,	MENU_SENS_END,		MENU_SENS_ALT);	//���������� ���������
MAKE_MENU(mSensTuneNumSens,	csSensorSet,	ssSensNext,		NULL_ENTRY,		SensSetNext,		NULL_FUNC,		mSensSwitch,		NULL_ENTRY);	//����� �������
MAKE_MENU(mSensSwitch,		csSensorSet,	ssSensSwitch,	NULL_ENTRY,		SensSwitch,			SensTestON,		MENU_SENS_END,		mSensPreAdr);	//���������-���������� �������
MAKE_MENU(mSensPreAdr,		csSensorSet,	ssSensPreAdr,	mSensAdrSet,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//��������� � ����� ������ �������
MAKE_MENU(mSensAdrSet,		csSensorSet,	ssSensAdr,		NULL_ENTRY,		SensAdrSet,			NULL_FUNC,		mSensPreName,		NULL_ENTRY);	//����� ������ ������� �� ����
MAKE_MENU(mSensPreName,		csSensorSet,	ssSensPreName,	mSensNameSet1,	NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//��������� � ����� �����
MAKE_MENU(mSensNameSet1,	csSensorSet,	ssSensName1,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensNameSet2,		NULL_ENTRY);	//����� ������ ����� �����
MAKE_MENU(mSensNameSet2,	csSensorSet,	ssSensName2,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensNameSet3,		NULL_ENTRY);	//����� ������ ����� �����
MAKE_MENU(mSensNameSet3,	csSensorSet,	ssSensName3,	NULL_ENTRY,		SensNameSet,		NULL_FUNC,		mSensWait,			NULL_ENTRY);	//����� ������� ����� �����
MAKE_MENU(mSensWait,		csSensorSet,	ssSensWaite,	mBaseClock,		NULL_FUNC,			NULL_FUNC,		mBaseClock,			NULL_ENTRY);	//�������� �������

void MenuIni(void){
	CurrMenuItem = (pgmMenuItem*)&mBaseClock;
}

void MenuStep(void){
	if (((FUNC_SWITCH)&SWITCH_MENU) == NULL_FUNC){							//��� ������� �������� ������������
		CurrMenuItem = &NEXT_MENU;											//������� �� ����� ����� ����
	}
	else{
		if (((FUNC_SWITCH)&SWITCH_MENU)() == 1)								//���� ������� ��������, ��������� � �� ����������� ��������� �� ������ ����� ����
			CurrMenuItem = &ALT_MENU;
		else
			CurrMenuItem = &NEXT_MENU;
	}
	SetClockStatus(CLOCK_STATUS, SET_STATUS);								//������������� �� ����� �����
}

void MenuOK(void){
	if (&BRANCH_MENU == &NULL_ENTRY){										//�� ��������� ������� �� ��������� ����, ������ ���������� �������
		if (((FUNC_MENU)&EXEC_MENU) != NULL_FUNC)							//�� ������ ������ ��������� ������� ������ �� �������, ����� �� ����
			((FUNC_MENU)&EXEC_MENU)();										//��������� �������
	}
	else {																	//������� �� ��������� ���� � ����� ������
		CurrMenuItem = &BRANCH_MENU;
		SetClockStatus(CLOCK_STATUS, SET_STATUS);
	}
}
