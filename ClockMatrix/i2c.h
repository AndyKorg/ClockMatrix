/*
 * i2c.h
 * ��������� � ����������� �� ���� i2c:
	1. RTC - DS3231SN
	2. ������ ����������� LM75AD
 * v.1.3
 */ 

#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>

#include "bits_macros.h"
#include "avrlibtypes.h"
#include "IIC_ultimate.h"
#include "EERTOS.h"
#include "Clock.h"
#include "CalcClock.h"

//---------  ������ ��������� �� ���� i2c. ������� ��� - ��� ���� ������-������ � ��������� i2c
#define RTC_ADR					0b11010000			//����� RTC
#define EXTERN_TEMP_ADR			0b10011110			//����� LM75AD

//*********  ������������ i2c ���������� RTC *********************
#define REFRESH_CLOCK		100						//������ ������ �������� �������� ������� �� ���������� �����
#define REPEAT_WRT_RTC		25						//������ ���������� ������ ���� ������� ������ �� �������

#define DisableIntRTC		ClearBit(INT_RTCreg, INT_RTC)
#define EnableIntRTC		SetBit(INT_RTCreg, INT_RTC)

//---------  ������ ��������� RTC
#define clkadrSEC		0x0						//����� �������� ������
#define clkadrMINUTE	0x1						//������
#define clkadrHOUR		0x2						//����
#define clkadrDAY		0x3						//����
#define clkadrDATA		0x4						//����
#define clkadrMONTH		0x5						//�����
#define clkadrYEAR		0x6						//���
#define alrm1adrSEC		0x7						//��������� 1 �������
#define alrm1adrMINUTE	0x8						//������
#define alrm1adrHOURE	0x9						//����
#define alrm1adrDATA	0xa						//���� ��� ����
#define alrm2adrMINUTE	0xb						//��������� 2 ������
#define alrm2adrHOUR	0xc						//����
#define alrm2adrDATA	0xd						//���� ��� ����
#define ctradrCTRL		0xe						//������� ��������. ���������� �������� � �����������
#define ctradrSTATUS	0xf						//������� ������� � ��������. ������ ������ RTC
#define ctradrAGGIN		0x10					//�������� ������������ ������������� ���������
#define ctradrMSB_TPR	0x11					//����� ����� ����������� � ������ �������������� ����.
#define ctradrLSB_TPR	0x12					//���� 7 � 8 ��� ������� ����� ����������� � ��������� 0,25 �������. �.�. 0 - ��� 0b00000000, 0b01000000 - 0.25, 0b10000000 - 0.5, 0b11000000 -0.75

//---------  ������ ������ ����������
#define maBitRTC		6						//����� ���� ������ ������������ ���������� � RTC - �� ���� ������ ��� �� �����
#define maDY_DT_MaskRTC	Bit(maBitRTC)			//����� ����� ������ ������������ ���������� � RTC
#define AlrmIsDYRTC(Mc)	BitIsSet(Mc, maBitRTC)
//---------  ���� ������������ ��������
#define DisableOSC_RTC	7						//������-��������� ���������� RTC. 1 - ���������� ���������
#define EnableWave_RTC	6						//��������� ����� ���������� �� ����� SQW
#define StartTXO_RTC	5						//��������� ��������� ����������� � ������
#define RS2_RTC			4						//������� ������� �� ������ SQW. �� ��������� 8.192 kHz
#define RS1_RTC			3
#define Int_Cntrl_RTC	2						//����� ������ INT ��� SQW. �� ��������� INT (��� �������)
#define EnableAlrm2RTC	1						//����� ���� ���������� ���������� �� ���������� 2
#define EnableAlrm1RTC	0						//����� ���� ���������� ���������� �� ���������� 1
#define Alrm1IsOnRTC(Mc) BitIsSet(Mc, EnableAlrm1RTC)
#define Alrm2IsOnRTC(Mc) BitIsSet(Mc, EnableAlrm2RTC)
//--------- ���� �������� ������� RTC
#define RTC_OSF_FLAG	7						//��������� ����������, � ������ ������ ������������
#define RTC_TEMP_BUSY	2						//RTC ����� �������� �����������
#define RTC_32KHZ_ENBL	3						//��������� ������ ���������� 32 ���

extern volatile u08 StatusModeRTC;				//������� ����� ������ � RTC �����������. ������� 5 ��� ��� ������� ����� ������ � RTC
#define WR_MODE_BIT		7						//��� ������ ������-������ RTC
#define ModeRTCIsRead	BitIsClear(StatusModeRTC, WR_MODE_BIT)
#define ModeRTCIsWrite	BitIsSet(StatusModeRTC, WR_MODE_BIT)
#define SetModeRTCRead	ClearBit(StatusModeRTC, WR_MODE_BIT)
#define SetModeRTCWrite	SetBit(StatusModeRTC, WR_MODE_BIT)
#define CALC_ADD_WR_BIT	6						//��� ����������� �����: Add ��� Dec. ����� ������� ������ � RTC ����� ������ ������������ � Add (����������� 1 � ��������� �������������� ��������)
#define CalcIsAdd		BitIsSet(StatusModeRTC, CALC_ADD_WR_BIT)
#define CalcIsDec		BitIsClear(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcAdd		SetBit(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcDec		ClearBit(StatusModeRTC, CALC_ADD_WR_BIT)
#define SetCalcDirect	SetBit(StatusModeRTC, CALC_ADD_WR_BIT)

#define SetWrtAdrRTC(Adr)	do {StatusModeRTC = ((StatusModeRTC & 0b11100000) | (Adr & 0b00011111));} while (0); //���������� ����� ������ � RTC
#define GetWrtAdrRTC	(StatusModeRTC & 0b00011111)

typedef	void (*SECOND_RTC)(void);				//��� ������� ���������� ������ ������� �� ������������ �������� RTC.

void Init_i2cRTC(void);							//������������� RTC
void SetSecondFunc(SECOND_RTC Func);			//���������� ������� ���������� ������ �������
SECOND_RTC GetSecondFunc(void);					//�������� ������ �� ������� ���������� ������ �������
void WriteToRTC(void);							//������ ������ � RTC
void Set0SecundFunc(SECOND_RTC Func);			//���������� ������� ���������� ������ ������� ������� ��� ����������
u08 i2c_RTC_DirectWrt(void);					//������ ������ ��������� Watch � RTC, ���������� 1 ���� ������ ������� ����������

//extern volatile u08 TemprCurrent;				//������� �������� ����������� � �������������� ����. ���� RTC � �������� ����������� � ��������� �� 0.25 ��������, ������������ ������ ����� ��������
extern volatile u08 OneSecond;					//������ �������� 0-1 ������ �������

//*********  ������������ i2c ���������� ������� ����������� *********************
#define REFRESH_EXT_TMPR	105					//������ ���������� ������ �������� ����������� �� �������� ������� �����������. ������� 105 �� ����� ���������� � ������� �� RTC ������� �������� � �������� 105 ��
#define REPEAT_EXT_TMPR		25					//������ ���������� ������ ���� ������� ������ �� �������
#define i2c_ExtTmprBuffer	2					//���������� �������� ���� �� ������ ����������� �� ���� �����

//---------  ������ ��������� ������� �����������
#define extmpradrTEMP		0x0					//�����������
#define extmpradrCONF		0x1					//������������
#define extmpradrTHYST		0x2					//����������� ����������� ��� ���� OS
#define extmpradrTOS		0x3					//����������� ������������ ���� OS
//---------  ���� ����������������� ��������
#define SleepExtTmpr		0					//���������� ��������� �����������, �������� ��������� ���������� ��������
#define OSPinModeExtTmpr	1					//����� ������ ���� OS. 0 - ������� ����������, 1 - ������ � ���� ��������.
#define OSPolModeExtTmpr	2					//0 - ���������� �������� ����������� �������� � ������ ������� ������ �� ���� OS, 1 - �������� ������� �������
#define OSQueModeExtTmpr0	3					//���������� ���������� ����������� ��� ���� OS
#define OSQueModeExtTmpr1	4

void Init_i2cExtTmpr(void);						//������������� ��������� ������� �����������
void i2c_ExtTmpr_Read(void);					//������ ��������� ����������� ���������

#endif /* I2C_H_ */