/*
 * ������ � ������������
 * ver. 1.1
 */ 

#ifndef ALARM_H_
#define ALARM_H_

#include "bits_macros.h"
#include "avrlibtypes.h"
#include "Clock.h"

#define ALARM_MAX					9		//���������� �����������
#define ALARM_EVRY_WEEK_NUM			4		//���������� ����������� � ������������ ������
#define ALARM_DURATION_MAX			31		//������������ ���������� ����� ������������ ����������
#define AlarmIsEvryWeek(Num)		(Num<ALARM_EVRY_WEEK_NUM)	//��������� �� ���� ������
#define AlarmIsDate(Num)			(Num>=ALARM_EVRY_WEEK_NUM)	//��������� �� ����

struct sAlarm{
	u08 Id;								//����� ����������
	u08 EnableDayRus;					//���������� ������������ �� ���� ������.����: 0 - ���������-���������� ����������, 1-��,2-��,3-��,4-��,5-��,6-��,7-��.
	u08 Duration;						//������������ �������� ���������� � ��������. �� 1 �� 99. ������ ������� ��� ������ ����� 0, �.�. �� ������������ ��� ���� ����� � CurrentDuration
	u08 CurrentDuration;				//���������� ������������ �������� ����� ������������ ����������. ������� ��� ������������ ��� ���� ��������� ����� ���������� ������������� �� �����. ��������� �� ����� �� �������� ������ ������������ �������� �������.
	struct sClockValue Clock;			//����� ������������
};
//TODO:�� ���������� �������� ��������� �� ����� � �������������� ���������� CurrentDuration

//------------- ����� EnableDayRus
//���������� �������� ������������ �����������
#define ALARM_ON_BIT				0												//����� ���� ���������-���������� ���������� � EnableDay.
#define AlarmOn(al)					SetBit((al).EnableDayRus, ALARM_ON_BIT)			//��������� ��������
#define AlarmOff(al)				ClearBit((al).EnableDayRus, ALARM_ON_BIT)		//��������� ���������
#define AlarmIsOn(al)				BitIsSet((al).EnableDayRus, ALARM_ON_BIT)		//��������� �������?
#define AlarmIsOff(al)				BitIsClear((al).EnableDayRus, ALARM_ON_BIT)		//��������� ��������?
#define ALARM_START_DAY				1												//������ ���� ������ ������������ ����������
//���������� ����������� � ������ ���� ������.
#define AlrmDyIsOn(al, dy)			BitIsSet((al).EnableDayRus, dy)					//�������� ��������� � ���������� ����
#define AlrmDyIsOff(al, dy)			BitIsClear((al).EnableDayRus, dy)				//�������� ���������� � ���������� ����
#define AlrmDyOn(al, dy)			SetBit((al).EnableDayRus, dy)
#define AlrmDyOff(al, dy)			ClearBit((al).EnableDayRus, dy)

//------------- ����� �������� ���������� ��� Duration
#define ALARM_COUNT_DURAT_BITS		7												//���������� ��� � �������� ������������
#define ALARM_DURAT_MASK			0b01111111										//���� ������������ ������� ���������� �����������
#define AlarmDuration(al)			((al).Duration & ALARM_DURAT_MASK)				//����������� ������������ ������� ����������
#define AlarmCurrentDur(al)			((al).CurrentDuration & ALARM_DURAT_MASK)		//������� ������������ �������� ����������. ������� ��������� �������. �� Duration �� 0
#define AlarmStartDurat(al)			do{(al).CurrentDuration = (al).Duration & ALARM_DURAT_MASK;} while (0)	//������ ������ ������������ ��������
#define AlarmPauseDurat(al)			SetBit((al).CurrentDuration, 7)					//������ ������������ ���������� �� �����
#define AlarmIsNotPause(al)			BitIsClear((al).CurrentDuration, 7)				//������ �� �� �����
#define AlarmStopDurat(al)			do{(al).CurrentDuration = 0;}while(0)			//���������� ������ ������������

void AlarmIni(void);
void AlarmCheck(void);						//�������� ������������� ��������� ����������
u08 TestAlarmON(void);						//��������� ��������� �������� ���������� 1-�������, 0-��������
struct sAlarm *FirstAlarm(void);			//���������� ��������� �� ������ ��������� � ������� �����������
struct sAlarm *ElementAlarm(u08 NumAlarm);	//���������� ��������� �� ��������� ����� NumAlarm
void SetNextShowAlarm(void);				//������� ��������� ��������� � �������� ��������
void SwitchAlarmStatus(void);				//��������-��������� ������� ���������
u08 LitlNumAlarm(void);						//�������� ������ �������� ���������� �� 0 �� 3
void ChangeCounterAlarm(void);				//����������� ��� ��������� ����� �������� �������� �������� ����������
void AddDurationAlarm(void);				//����������� ������������ ������� ���������� �� 1
void AlarmDaySwitch(void);					//��������-��������� ��������� � ������������ ���� ������

#endif /* ALARM_H_ */