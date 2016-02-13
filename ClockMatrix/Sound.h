/*
 * ��������������� �����
 */ 

// TODO �� ���������� ������ ����� ���������� ����� ���������� - ������ ���������� ��� ������ �� �����

#ifndef SOUND_H_
#define SOUND_H_

#include "pff.h"
#include "avrlibtypes.h"
#include "bits_macros.h"

#define ALARM_FILE_NAME			"alarm.wav"	//��� ����� ��� ����� ����������
#define EACH_HOUR_FILE_NAME		"each.wav"	//��� ����� ��� ����� ������ ���
#define ONE_HOUR_FILE_NAME		"one.wav"	//��� ����� ��� ����� ���. ������ ������� ��� ������� � ��������� ����� �����

//���� ��������
#define SND_KEY_BEEP			0			//���� ������
#define SND_ALARM_BEEP			1			//���� ����������
#define SND_EACH_HOUR			2			//���� ��������
#ifdef VOLUME_IS_DIGIT
	#define SND_TEST			3			//�������� ����, ������������ ��� ��������� ���������
	#define SND_TEST_EACH		4			//���� ����� ��������
	#define	SND_TEST_ALARM		5			//���� ����� ����������
#endif
#define SND_SENSOR_TEST			10			//���� ��� ��������� ������ ������ �� �������� �������

//------- �������� ������� ��� �����, ���������� ������ 512, �.�. Petit_FatFs �������� ������ �������� ������ � ������ �������
#define BUF_LEN			512//256
#if (BUF_LEN != 512)
	#warning "The size of the buffer of memory not 512 bytes, are possible operation deceleration"
#endif

extern FATFS fs;							//FatFs object
extern unsigned char read_buf0[BUF_LEN], read_buf1[BUF_LEN];//���� ������ ������� �� �����, ������� ������ �.�. ������������ � ������ �������
extern volatile unsigned char	buf1_empty, buf0_empty;		//��������� �������

void SoundIni(void);						//������������� �������� �������
void EachHourSettingSetOff(void);			//��������� �������
void EachHourSettingSwitch(void);			//����������� ���������� �� �������
u08 EachHourSettingIsSet(void);				//��������� ���� �������� ������� ������ ���. 0 - ��������
void KeyBeepSettingSetOff(void);			//��������� ���� ������
void KeyBeepSettingSwitch(void);			//����������� ���������� ����� ������
u08 KeyBeepSettingIsSet(void);				//��������� ���� �������� ������. 0 - ���� ������ ��������
void SoundOn(u08 SoundType);				//�������� ������ ���������� ����
void SoundOff(void);						//��������� ������
u08 AlarmBeepIsSound(void);					//������ ���������� � ������� ������ ������? 0 - ���, 1 - ��
u08 SoundIsBusy(void);						//�������� ������� ������ fatFs 0 - ��������, 1 - ������

#endif /* SOUND_H_ */
