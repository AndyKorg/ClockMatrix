/*
 * ������������ ������� �������� BNP180
 *
 */ 


#ifndef BMP180HAL_H_
#define BMP180HAL_H_

#include "avrlibtypes.h"
#include "eeprom.h"

//��������
#define BMP180_CONTROL_REG			0xf4		//������� ����������
#define BMP180_RESET_REG			0xe0		//������� ������������ ������.
#define BMP180_CHIP_ID_REG			0xd0		//������� id ����
#define BMP180_DATA_REG_START		0xf6		//��������� ����� �������� ������
//������ ������������� �������������, _S_ - �� ������, _U_ - ��� �����
#define BMP180_COFF_COUNT			22			//���������� ���� ��� ������������� �������������
#define BMP180_COEFF_ADR_START		0xAA		//��������� ����� ����� ��������� ����������� �������������
#define BMP180_COEFFCNT_S_AC1		0			//����� ������� ����� ������������ AC1, ��� ������������ 16-������, ������ ���� ������� ����. BMP180_COEFFCNT_S_AC1+1 ������� ����
#define BMP180_COEFFCNT_S_AC2		2
#define BMP180_COEFFCNT_S_AC3		4
#define BMP180_COEFFCNT_U_AC4		6
#define BMP180_COEFFCNT_U_AC5		8
#define BMP180_COEFFCNT_U_AC6		10
#define BMP180_COEFFCNT_S_B1		12
#define BMP180_COEFFCNT_S_B2		14
#define BMP180_COEFFCNT_S_MB		16
#define BMP180_COEFFCNT_S_MC		18
#define BMP180_COEFFCNT_S_MD		20
//�������
#define BMP180_SOFT_RESET			0xb6		//������� ������ ��� �������� BMP180_RESET_REG
//������� ������� ���������� ��� �������� BMP180_CONTROL_REG
#define BMP180_TEPERATURE_START		0x2e		//������ ��������� �����������. ����� ��������� 4.5 ��
#define BMP180_PRESRE_LOW_START		0x34		//�������� � ������ oss=0 (����� ������ �����������), ����� ��������� 4.5 ��
#define BMP180_PRESRE_NORMAL_START	0x74		//� ������ oss = 1 (����������� �����), ����� ��������� 7.5 ��
#define BMP180_PRESRE_HI_RES_STAT	0xb4		//������� ���������� - oss=2, 13,5 ��
#define BMP180_PRESRE_UL_RES_START 	0xf4		//������������ ����������, oss=3, 25.5 mc
//������������ ���������, ��
#define BMP180_TEMPERATURE_TIME		5			//4.5 �� �� ����� ����
#define BMP180_PRESRE_LOW_TIME		5			//����� ��������� 4.5 ��
#define BMP180_PRESRE_NORMAL_TIME	8			//� ������ oss = 1 (����������� �����), ����� ��������� 7.5 ��
#define BMP180_PRESRE_HI_RES_TIME	14			//������� ���������� - oss=2, 13,5 ��
#define BMP180_PRESRE_UL_RES_TIME 	26			//������������ ����������, oss=3, 25.5 mc

#define BMP180_CMD_LEN				1			//����� ������� 1 ����
#define	BMP180_COFF_LEN				2			//����� ������ ������������� �����.
#define	BMP180_TEMPER_LEN			2			//����� ������ �����������
#define	BMP180_PRESS_LEN			3			//����� ������ ��������

#define BMP180_REPEAT_TIME_MS		50			//������ ���������� ������� ��������� ���� ���� I2C ������

//������������ ����������
#define BMP180_PRESSURE_START		BMP180_PRESRE_NORMAL_START	//<<<<<------- �������� ���� ��������� ������ ��������
//����������� ������� ���������
#if (BMP180_PRESSURE_START == BMP180_PRESRE_LOW_START)
	#define BMP180_OSS				0
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_LOW_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_NORMAL_START)
	#define BMP180_OSS				1
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_NORMAL_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_HI_RES_STAT)
	#define BMP180_OSS				2
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_HI_RES_TIME
#elif (BMP180_PRESSURE_START == BMP180_PRESRE_UL_RES_START)
	#define BMP180_OSS				3
	#define BMP180_PRESSURE_TIME	BMP180_PRESRE_UL_RES_TIME
#else
	#error "Undefine BMP180_PRESSURE_START"
#endif

#define  BMP180_INVALID_DATA		0x1fff		//������ ��������� �������� � �����������
typedef void (*PRESS_AND_TEMPER)(u16*, u16*);	//������� ���������� �� ���������  ��������� ��� ��������

void StartMeasuringBMP180(void);

PRESS_AND_TEMPER PressureRead;

#endif /* BMP180HAL_H_ */