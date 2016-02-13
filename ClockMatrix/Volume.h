/*
 * ����������� ���������
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

#include "avrlibtypes.h"

#define VOL_TYPE_COUNT		4				//���������� �������������� ����� ������������� ���������
//���� ��������� ���������� �����
enum tVolumeType{
	vtButton = 0,							//��������� ����� ������
	vtEachHour = 1,							// -/-       -/-  ������ ���
	vtAlarm = 2,							// -/-       -/-  ����������
	vtSensorTest = 3						// -/-       -/-  ��������� ��������� ������� �� ����������� ������� ��������
	};

//���� ������������� ���������
enum tVolumeLevel{
	vlConst = 1,							//���������� ������������� ������� ���������
	vlIncreas = 2,							//����������� ������� �� �������������� ������
	vlIncreaseMax = 3,						//����������� ������� �� ������������� ������
	vlLast = 4,								//��������� �������, ������������ ������ ��� ������� ����� ������������
	};

#define VOLUME_LEVEL_MAX	64				//������������ �������� ��������� ���� �������� ������������� ��������� �� �������
#define VOL_CHANGE_LVL_TIME	250				//����� ����� ������ ��� ������� ���������� ���������, ��

//�������� ��������� ��� ������������� ����
struct sVolume{
	enum tVolumeLevel LevelVol;				//��� �������������
	u08 Volume;								//�������� ��������� ��� ����� ����
	};

extern struct sVolume VolumeClock[VOL_TYPE_COUNT];

void VolumeIni(void);						//������������� ���������� ���������
void VolumeKeyBeepAdd(void);				//��������� ��������� ����� ��� ������
void VolumeEachHourAdd(void);				//����������� ��������� ����������� ���
void VolumeAlarmAdd(void);					//����������� ��������� ����������
void VolumeTypeTuneAlarm(void);				//������������ ���� ����������� ��������� ��� ����������
void VolumeOff(void);						//���������� �����
void VolumeOn(void);						//��������� �����
u08 VolumeAlrmTypeIsNeedLevel(void);		//��������� ��� ����������� ������ ��������� ����������

void VolumeAdjustStart(enum tVolumeType Value);//��������� ������������� ������ ��������� ��� ������� ����

#endif /* INCFILE1_H_ */