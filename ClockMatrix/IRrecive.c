/*
 * �������� � ������������ ����� IRrecive.h
 * ver 1.4
 */ 

#include <stddef.h>
#include "Alarm.h"
#include "i2c.h"
#include "IRrecive.h"
#include "MenuControl.h"
#include "Sound.h"
#include "EERTOS.h"
#include "Display.h"
#include "keyboard.h"
#include "sensors.h"

#ifdef IR_SAMSUNG_ONLY
	#include "IRreciveHAL.h"
#else
	#include "IRMultiHAL.h"
#endif

#define IR_INDICATOR_X	0							//���������� ����� ������������� ������ ������ �� �� ���������
#define IR_INDICATOR_Y	7
#define IR_DURAT_BLINK	200							//������������ �������� ���������� ��������� �� ������
#define IR_COUNT_BLINK	((u08) 1000/IR_DURAT_BLINK)	//���������� �������� ���������� �� 1000 ��

volatile u16 PeriodIR;								//����������� ������ ����� ���������� �� ���������, ������������ � ������������� ������� � ��-������
u32 IRcmdList[IR_NUM_COMMAND];						//���� �� ������

VOID_PTR_VOID IRPeriodEvent = NULL;					//������ �� ������� ��������� ������� PERIOD_INT_IR
RECIV_IR_PTR IRReciveReady = NULL;					//������ ���� ������� ������� ��� ��������� �������� �������. �.�. ������� ���������� � ����������, �� ��� ������ ���� ���������� �������

void IRReciveRdy(u08 AdresIR, u32 CommandIR);
/************************************************************************/
/* ��������� ������� ��������� ������ �� �� ������.						*/
/************************************************************************/
void IRReciveRdyOn(void){
	IRReciveReady = IRReciveRdy;
}
/************************************************************************/
/* �������� ����� � ������� BDC ���� � ������� ����� ���� � �������     */
/************************************************************************/
u08 AddDigitNibble(u08 Value, u08 Digit, u08 HighNibbl){
	if (HighNibbl)
		return ((Value & 0xf0) | (Digit<<4));
	else
		return ((Value & 0x0f) | Digit);
}

/************************************************************************/
/* ��������� ������ ������ �� �� ���������                              */
/************************************************************************/
void IRIndicatorBlink(void){
	static u08 Count = IR_COUNT_BLINK;
	plot(IR_INDICATOR_X, IR_INDICATOR_Y, DotIsSet(IR_INDICATOR_X, IR_INDICATOR_Y));
	if (Count){
		Count--;
		SetTimerTask(IRIndicatorBlink, IR_DURAT_BLINK);
	}
}

/************************************************************************/
/* ����� ��������� ������ ������ �� �� ������							*/
/************************************************************************/
void IRStartIndicator(void){
	plot(IR_INDICATOR_X, IR_INDICATOR_Y, 1);			//�������� ��������� ������ ������ �� ��
	SetTimerTask(IRIndicatorBlink, IR_DURAT_BLINK);
}

/************************************************************************/
/* �������� ������ ������� �� �� ����                                   */
/************************************************************************/
#define IR_CMD_UNKNOWN	0xff							//������� �� ��������. ������������ ��� 0xff ��������� ����� ��� ������� ���� ������ ����������.
inline u08 DecodeCmd(u32 Cmd){
	for(u08 i=0; i<IR_NUM_COMMAND; i++)
		if (IRcmdList[i] == Cmd)
			return i;
	return IR_CMD_UNKNOWN;
}

/************************************************************************/
/* ���������� ������ ��������� �� ����� ��������� �������               */
/************************************************************************/
void IRStartFromDelay(void){
	StartIRCounting();									//��������� ������� ��������� ����� ��������� ���������
}

/************************************************************************/
/* ��������� ������ ���������� �� �� ������ �� ��������� samsung        */
/************************************************************************/
void IRReciveRdy(u08 AdresIR, u32 CommandIR){
		
	if (AdresIR == REMOT_TV_ADR){							//������� � ������ samsug
		u08 IrCmd = DecodeCmd(CommandIR);
		if	(
			(AlarmBeepIsSound() == 0)						//������ ������ ���� ���� ���������� ��� �� �������
			&&
			(IrCmd != IR_PLAY_START)						//� �� ������ �����
#ifndef IR_SAMSUNG_ONLY										//���� ������������ ������������ �� �����, �� ������ ������ ��� ���������� �����. ������ ��� ���� ��� �� �� ������ �� ������ �����
			&&
			(IrCmd != IR_CMD_UNKNOWN)
#endif		
			)
			SoundOn(SND_KEY_BEEP);							//�������� ���� ���������
		if (IrCmd != IR_CMD_UNKNOWN){						
			IRStartIndicator();								//������� �������� ������� �����
			if (ClockStatus == csAlarm){
				SetClockStatus(csClock, ssNone);			//��������� ����� ���������� ���� �������
				SoundOff();									//���� ���������
			}
			else{											//������� �����, ������ ���������� �������
				TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);//������� ������� �������� �������� � �������� �����
				switch (IrCmd){
					case IR_PLAY_START:
						if (AlarmBeepIsSound() == 0)			//������������� ������� ����������, ���� ��� ��� �� ������
							SoundOn(SND_ALARM_BEEP);
						break;
					case IR_PLAY_STOP:
						if (AlarmBeepIsSound() == 1)			//���������� ���������������, ���� ��� ����
							SoundOff();
						break;
					case IR_OK:									//���� ��
						MenuOK();
						break;
					case IR_STEP:								//���� Step
						MenuStep();
						break;
					case IR_INC:								//��������� ����� ������ - ����� ��������� ����� � ��������
					case IR_DEC:								//�������� ��������
						if (
							 (
								(ClockStatus == csSet) ||		//��� � �����
								(ClockStatus == csAlarmSet)		//��� � �����������
							 )
							&& (SetStatus != ssNone)			//� ������ ���� ������ ���������� �������
						   ){			
							if (IrCmd == IR_INC)
								SetCalcAdd;
							else
								SetCalcDec;
							if (ClockStatus == csSet)			//�������� ����� ������� � ���������� RTC
								WriteToRTC();
							else
								ChangeCounterAlarm();			//�������� ������� ����������
						}
						break;
					default:
						break;
				}//switch
			}
		}
		SetTimerTask(IRStartFromDelay, SCAN_PERIOD_KEY);
	}//if (AdresIR == REMOT_TV_ADR)
#ifdef IR_SAMSUNG_ONLY
	else{												//�����-�� ������ ������, �� �� ������ ��
		SetSensor(AdresIR, AdresIR, CommandIR);			//���� ������� ��� ��� ������ �� �������� ������� �����������. ��������� ���� AdresIR ���������� �� ������� �������� � ������ � �����, �� � ������ ������, �� �� ������ ���������� ��� ����.
	}
#else
	#warning "IR reciver for sensors is off"
#endif
}

void IRReciverInit(void){
	IRRecivHALInit();
	IRReciveReady = IRReciveRdy;						//���������� ������� ��������� �������� ������ ��
}
