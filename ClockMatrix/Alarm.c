/*
 * ������ � ������������
 * ver. 1.1
 */ 

#include <avr\interrupt.h>
#include "Alarm.h"
#include "CalcClock.h"
#include "Sound.h"
#include "eeprom.h"
#include "esp8266hal.h"

struct sAlarm Alarms[ALARM_MAX];											//����������

/************************************************************************/
/* �������� ����������� �� ������������.								*/ 
/*  ������ ���������� ������ ������� ������� � ������                   */
/************************************************************************/
#define AlarmCheckEqu()	((Alarms[i].Clock.Hour == Watch.Hour) && (Alarms[i].Clock.Minute == Watch.Minute) && (Watch.Second == 0)) //�������� ���������� ���������� ���������� � �������� �������
void AlarmCheck(void){
	u08 i;

	if (ClockStatus == csAlarm){											//����� ������������ ����������, ���� ��������� ������������� ���������� ����� ����������
		for (i=0; i<ALARM_MAX; i++){										//�������� ������������� ���������� ���������� ��� ���� �����������
			if AlarmIsNotPause(Alarms[i]){									//���� ���������� �� �� �����
				if AlarmCurrentDur(Alarms[i]){								//������� ������������ �������� �� 0, � ������ ���� ��������� ������ � ������ ������
					Alarms[i].CurrentDuration--;							//�������� 1 ������
					if (AlarmCurrentDur(Alarms[i]) == 0){					//���� �������� �������� ������, ��������� ���������
						SoundOff();											//��������� ����
						SetClockStatus(csClock, csNone);					//�������� � ���������� �����
					}
				}
			}
			else{															//���� ���������� �� �����, ������ ��� ���������� ������ �����
																			//��� �� �����������
			}
		}
	}
	
	for(i=0;i<ALARM_MAX; i++){												//�������� ������������� ��������� ������� ���������� ��� ���� �����������
		if (AlarmIsOn(Alarms[i]) && (AlarmCurrentDur(Alarms[i]) == 0)){		//���� ��������� �������? � ��� �� ����������
			if AlarmCheckEqu(){												//����� ���������
				if AlarmIsEvryWeek(i){										//��� ��������� �� ���� ������
					if AlrmDyIsOff(Alarms[i], what_day(Watch.Date, Watch.Month, Watch.Year)){	//� ���� �� ���������
						continue;											//��������� �� ��������	
					}
				}
				else if (AlarmIsDate(i) &&
						((Alarms[i].Clock.Date != Watch.Date) ||
						(Alarms[i].Clock.Month != Watch.Month))){				//��������� �� ���� � ���� �� ��
					continue;
				}
				SetClockStatus(csAlarm, csNone);								//���������� ������������ ����������
				AlarmStartDurat(Alarms[i]);										//������ ������ ������������ ������� ����������
				SoundOn(SND_ALARM_BEEP);										//���� ��������
			}
		}
	}

	if (Watch.Minute == 0){													//������ ������ ���
		i = HourToInt(Watch.Hour);											//������������� � ���������� �����
		if ((i >= EACH_HOUR_START) && (i<=EACH_HOUR_STOP))					//������� ������ � ��������� ���������� �������
			SoundOn(SND_EACH_HOUR);
	}
}

/************************************************************************/
/*  ��������-��������� ��������� � ������� ����                         */
/************************************************************************/
void AlarmDaySwitch(void){
	if (AlrmDyIsOn(*CurrentShowAlarm, CurrentAlarmDayShow))
		AlrmDyOff(*CurrentShowAlarm, CurrentAlarmDayShow);
	else
		AlrmDyOn(*CurrentShowAlarm, CurrentAlarmDayShow);
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* �������� 1 � ������������ �������� �������� ����������               */
/************************************************************************/
void AddDurationAlarm(void){
	if ((*CurrentShowAlarm).Duration == ALARM_DURATION_MAX)
		(*CurrentShowAlarm).Duration = 1;
	else
		(*CurrentShowAlarm).Duration += 1;
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ����������� ������� ������� �������� ����������                      */
/************************************************************************/
void ChangeCounterAlarm(void){
	volatile u08 *Tmp = &((*CurrentCount).Minute);
	
	switch (SetStatus){
		case ssMinute:
			Tmp = &((*CurrentCount).Minute);
			break;
		case ssHour:
			Tmp = &((*CurrentCount).Hour);
			break;
		case ssDate:
			Tmp = &((*CurrentCount).Date);
			break;
		case ssMonth:
			Tmp = &((*CurrentCount).Month);
			break;
		case ssYear:
			Tmp = &((*CurrentCount).Year);
			break;
		default:
			break;
	}
	*Tmp = CalcIsAdd?AddClock(CurrentCount, SetStatus):DecClock(CurrentCount, SetStatus);
	SetCalcAdd;													//�� ��������� ������ ���������� 1
	AlarmStopDurat(*CurrentShowAlarm);							//������� ������������ ��������
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ���������� 1 ���� ������� ����� ����������							*/
/* �� 0 �� ALARM_EVRY_WEEK_NUM (������������ ������������),				*/
/* ��� 0 ���� �� ALARM_EVRY_WEEK_NUM �� ALARM_MAX (�� �����)			*/
/************************************************************************/
u08 LitlNumAlarm(void){
	return (CurrentShowAlarm->Id <= ALARM_EVRY_WEEK_NUM-1)?1:0;
}

/************************************************************************/
/* ���������� ��������� �� ������ ��������� � ������� �����������		*/
/************************************************************************/
struct sAlarm *FirstAlarm(void){
	return 	&Alarms[0];
}

/************************************************************************/
/* ���������� ��������� �� ��������� ����� NumAlarm                     */
/************************************************************************/
struct sAlarm *ElementAlarm(u08 NumAlarm){
	if (NumAlarm<ALARM_MAX)
		return &Alarms[NumAlarm];
	else
		return &Alarms[0];
}


/************************************************************************/
/* ����� ���������� ����������                                          */
/************************************************************************/
void SetNextShowAlarm(void){
	if (CurrentShowAlarm->Id == ALARM_MAX-1)
		CurrentShowAlarm = &Alarms[0];
	else
		CurrentShowAlarm = &Alarms[CurrentShowAlarm->Id+1];
	CurrentCount = &(CurrentShowAlarm->Clock);					//������� ������� ������� ����� ������� ����������
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ��������� - ���������� �������� ����������                           */
/************************************************************************/
void SwitchAlarmStatus(void){
	if AlarmIsOn(*CurrentShowAlarm){
		AlarmOff(*CurrentShowAlarm);
	}
	else
		AlarmOn(*CurrentShowAlarm);
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ��������� ��������� ����������: �������� ��� ��������                */
/************************************************************************/
u08 TestAlarmON(void){
	return AlarmIsOn(*CurrentShowAlarm)?1:0;
}

void AlarmIni(void){
	u08 i;

	for(i=0; i<ALARM_MAX; i++){								//��������� ����������� �� ���������
		Alarms[i].Id = i;
		if (EeprmReadAlarm(&Alarms[i]) == 0){				//�� ������� ��������� ��������� ���������� �� eeprom, ������������� �� ���������
			AlarmOff(Alarms[i]);							//��������� ���������
			Alarms[i].Duration = 1;							//������������ ������� 1 ������
			if (i>=ALARM_EVRY_WEEK_NUM){					//��� ����������� �� ����� ���������������� ���������� ����
				Alarms[i].Clock.Date = 0x01;
				Alarms[i].Clock.Month = 0x01;
				Alarms[i].Clock.Year = 0x15;				//���� ��� � ������������, ��� ����� ��� ��������������
			}
			AlarmStopDurat(Alarms[i]);						//������� ������������ ��������
		}
	}
	CurrentShowAlarm = &Alarms[0];
}
