/*
 * keyboard.c
 *	������������ ����������
 *	����� ������ ��������� ���������� ��������.
 *	��� �������� ����������� ����� ������ ������� ���� MenuStep � MenuOK
 *  ��� �� ������������ ������� SoundOn � SoundOff ��� ��������� ����� ��� �������
 */ 


#ifdef EXTERNAL_INTERAPTS_USE
	#include <avr/interrupt.h>
#endif

#include "keyboard.h"
#include "EERTOS.h"
#include "bits_macros.h"
#include "i2c.h"
#include "MenuControl.h"
#include "Alarm.h"
#include "Sound.h"


volatile u08 Key = 0;										//��������� ������
volatile u08 PrevKey = 0;									//����������� ��������� ���������� ����� �������� ����������

#ifndef EXTERNAL_INTERAPTS_USE
	void KeyScan(void);										//������� ������������ ������ ���� �� ������������ ����������
#endif

/************************************************************************/
/* �������� ������������� �������� � �������� ����� �� �������� �		*/
/* ��������� ��������� ��������                                         */
/* ���������� � ���������� Minuts = 0 ��� �������� �������������		*/
/* � �������� �����, ���� ���������� ��� ��������� �����-���� ������� ��*/
/* ���������� ��� ��������� �� � ����������� ����� ��������				*/
/************************************************************************/
void TimeoutReturnToClock(u08 Minuts){
	static u08 CountWaitingEndSet;							//������ ��������� � ������� �������� �� ���� ������� ������ ��� ������ �� ��-������.
	
	if (Minuts)												//���������� ������� ��������
		CountWaitingEndSet = Minuts;
	else{													//��������� ������������� �������� � �������� �����
		if (CountWaitingEndSet)								//�������� ������������� �������� � �������� ����� ����� �� ��������
			CountWaitingEndSet--;							//���� ��� ���������
		else{
			if ((ClockStatus != csClock)					//����� ����������� �������
				&& 
				(ClockStatus != csAlarm) 
				&& 
				(ClockStatus != csSecond)
				&&
				(ClockStatus != csInternetErr)
				){//������������ � �������� ����� �� ��������� �������� �����������
				MenuIni();									//����� ����
				SetClockStatus(csClock, ssNone);			//����� ������ �����
			}
		}
	}
}

/************************************************************************/
/*  ������������� ������� ������� � ��� ���������						*/
/*   ������� ����������� ����� �������� �������� ����� ����������		*/
/************************************************************************/
inline void BeginScakKeyRepeat(void){						//������ ��������� ���������� �������
#ifdef EXTERNAL_INTERAPTS_USE
	GICR |= KEY_INT_MASK;									
#else
	SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
#endif
}

void KeyCheck(void){

	if	(
		BitIsSet(PrevKey, KEY_STEP_FLAG)					//����� ��������� ���������� ����� STEP
		&&
		BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		)
		SetBit(Key, KEY_STEP_FLAG);
	if	(
		BitIsSet(PrevKey, KEY_OK_FLAG)						//����� ��������� ���������� ����� OK
		&&
		BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		)
		SetBit(Key, KEY_OK_FLAG);
	PrevKey = 0;
	if (Key){												//��, ���� ������� �����-�� �������
		if (AlarmBeepIsSound()){							//���� ������ ���������� ������� �� ��������� ���
			SoundOff();
			SetClockStatus(csClock, csNone);				//�������� � ���������� �����
			Key = 0;
			BeginScakKeyRepeat();
			return;											//� ����� �.�. ��� ������� ���������� ����������
		}
	TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);		//���������� �������� �������� ��� �������� � �������� ����� 
	SoundOn(SND_KEY_BEEP);									//�������� �������
	}
	if BitIsSet(Key, KEY_STEP_FLAG){						//���
		MenuStep();
	}
	else if BitIsSet(Key, KEY_OK_FLAG){						//��
		MenuOK();
	}
	Key = 0;												//��� ������� ����������
	BeginScakKeyRepeat();
}

#ifdef EXTERNAL_INTERAPTS_USE
/************************************************************************/
/* ���������� �� ������ STEP							                */
/************************************************************************/
ISR(KEY_STEP_INTname){
	if BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		SetBit(PrevKey, KEY_STEP_FLAG);
	ClearBit(GICR, KEY_STEP_INT);							//��������� ���������� �� ��� ����������� ������
	SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
}

/************************************************************************/
/* ���������� �� ������ ��												*/
/************************************************************************/
ISR(KEY_OK_INTname){
	if BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		SetBit(PrevKey, KEY_OK_FLAG);
	ClearBit(GICR, KEY_OK_INT);								//��������� ���������� �� ��� ����������� ������
	SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
}
#else
/************************************************************************/
/* ������������ ������                                                  */
/************************************************************************/
void KeyScan(void){
	if BitIsClear(KEY_PORTIN_STEP, KEY_STEP_PORT)
		SetBit(PrevKey, KEY_STEP_FLAG);
	if BitIsClear(KEY_PORTIN_OK, KEY_OK_PORT)
		SetBit(PrevKey, KEY_OK_FLAG);
	if (PrevKey)											//�����-�� ������� ������, ��������� ������� �������� ����� �������� ��������
		SetTimerTask(KeyCheck, PROTECT_PRD_KEY);
	else													//������ �� ������, ���������� �����������
		SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
}
#endif

/************************************************************************/
/* ������������� ��������� ������� �� ����������                        */
/************************************************************************/
void InitKeyboard(void){
	MenuIni();
	ClearBit(KEY_PORTIN_OK, KEY_OK_PORT);
	ClearBit(KEY_PORTIN_STEP, KEY_STEP_PORT);
#ifdef EXTERNAL_INTERAPTS_USE
	MCUCR |= (1<<ISC01) | (1<<ISC00) |						//�� ���������� ������ �� INTo
			 (1<<ISC11) | (1<<ISC10);						//�� ���������� �� INT1
	GICR |= (1<<INT0) | (1<<INT1);							//��������� ���������� �� ������
#else
	SetTimerTask(KeyScan, SCAN_PERIOD_KEY);
#endif
	PrevKey = 0;
	Key = 0;
}
