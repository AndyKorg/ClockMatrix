/*
 * ����������� ���������
 */ 

#include <stddef.h>
#include "EERTOS.h"
#include "Clock.h"
#include "VolumeHAL.h"
#include "Volume.h"

struct sVolume VolumeClock[VOL_TYPE_COUNT];				//��������� ������� ��������� ��� ������ �������
u08 IncreaseMaxVolume = 0,								//������� �� �������� ���� ��������� ��������� ��� ����������� ���������
	CurrenVolume = 0;									//������� ������� ���������

/************************************************************************/
/* ������������� ���������� ���������                                   */
/************************************************************************/
void VolumeIni(void){
	VolumeIntfIni();									//��������� ���� � ���������
	VolumeClock[vtAlarm].LevelVol = vlIncreaseMax;		//��������� ��������� �� ���������
	VolumeClock[vtAlarm].Volume = VOLUME_LEVEL_MAX;
	VolumeClock[vtButton].Volume = 3;
	VolumeClock[vtButton].LevelVol = vlConst;
	VolumeClock[vtEachHour].Volume = VOLUME_LEVEL_MAX>>1;
	VolumeClock[vtSensorTest].Volume = VOLUME_LEVEL_MAX;
	VolumeClock[vtSensorTest].LevelVol = vlConst;
}

void VolumeAddLevel(enum tVolumeType Value){
	if (VolumeClock[Value].Volume == VOLUME_LEVEL_MAX)	//������������ ���������?
		VolumeClock[Value].Volume = 0;
	else
		VolumeClock[Value].Volume++;
	VolumeSet(VolumeClock[Value].Volume);
	VolumeOnHard;
	if (Refresh != NULL)
		Refresh();
}
	
/************************************************************************/
/* ����������� ��������� ������� ������                                 */
/************************************************************************/
void VolumeKeyBeepAdd(void){
	VolumeAddLevel(vtButton);
}

/************************************************************************/
/* ����������� ��������� ����������� ���                                */
/************************************************************************/
void VolumeEachHourAdd(void){
	VolumeAddLevel(vtEachHour);
}
/************************************************************************/
/* ����������� ��������� ����������		                                */
/************************************************************************/
void VolumeAlarmAdd(void){
	VolumeAddLevel(vtAlarm);
}

/************************************************************************/
/* ������������ ���� ����������� ��������� ��� ����������               */
/************************************************************************/
void VolumeTypeTuneAlarm(void){
	VolumeClock[vtAlarm].LevelVol++;
	if (VolumeClock[vtAlarm].LevelVol == vlLast)//��������� ����� ������������?
		VolumeClock[vtAlarm].LevelVol = 0;
	if (Refresh != NULL)
		Refresh();
}

/************************************************************************/
/* ��������� ��� ����������� ������ ��������� ����������.               */
/* ���������� 0 ��� ����������� �� ������� �������� ������ ���			*/
/* 1 ���� ��������� �������� ������ ���������							*/
/************************************************************************/
u08 VolumeAlrmTypeIsNeedLevel(void){
	return (VolumeClock[vtAlarm].LevelVol == vlIncreaseMax)?0:1;
}

/************************************************************************/
/* ����������� ������ ��������� �� ��������� ������                     */
/************************************************************************/
void VolumeIncrease(void){
	if (CurrenVolume <= IncreaseMaxVolume){				//����������� ��������� ��� �� ����������, ����������
		CurrenVolume++;
		VolumeSet(CurrenVolume);
		SetTimerTask(VolumeIncrease, VOL_CHANGE_LVL_TIME);
	}
	else{												//����������� ��������� ����������, ���������������
		CurrenVolume = 0;								//����� � ����������� �����
		IncreaseMaxVolume = 0;
	}
}

/************************************************************************/
/* ��������� ������������� ������ ��������� ��� ������� ����            */
/************************************************************************/
void VolumeAdjustStart(enum tVolumeType Value){
	VolumeSet(0);
	VolumeOnHard;
	if (VolumeClock[Value].LevelVol != vlConst){			//����������� ����, ����������� ������ ���������� ������ ���������
		if (VolumeClock[Value].LevelVol == vlIncreas)
			IncreaseMaxVolume = VolumeClock[Value].Volume;
		else
			IncreaseMaxVolume = VOLUME_LEVEL_MAX;
		VolumeIncrease();									//������ ����������� ������ ������ ���������
	}
	else
		VolumeSet(VolumeClock[Value].Volume);
}

/************************************************************************/
/* ����������-��������� �����                                           */
/************************************************************************/
void VolumeOff(void){
	CurrenVolume = 0;										//������� ������� ���������
	VolumeOffHard;	
}

void VolumeOn(void){
	VolumeOnHard;
}
