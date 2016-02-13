/*
 * OneWare.c
 * ������������ �� ���� 1-Ware � ������� ����������
 * Created: 26.07.2014 15:24:18
 * ver. 1.2
 */ 


#ifndef ONEWARE_H_
#define ONEWARE_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "avrlibtypes.h"
#include "bits_macros.h"

typedef void (*ONEWARE_EVENT)(void);						//��� ��������� �� ������� ��������� ��������� ������� �� ���� 1-Ware

#define ONE_WARE_BUF_LEN	2
extern volatile u08	owStatus,								//������� ������ ���� 1-Ware, �������� ����������
					owRecivBuf[ONE_WARE_BUF_LEN],			//����� ��� �������� ����
					owSendBuf[ONE_WARE_BUF_LEN];			//����� ��� ������������ ����
//����� ���������� OneWareStatus
#define OW_BUSY			7									//��� ��������� ���� 1-ware, 0 - ��������, 1 ������
#define owBusy()		SetBit(owStatus, OW_BUSY)			//������ ����
#define owFree()		ClearBit(owStatus, OW_BUSY)			//���������� ����
#define owIsFree()		BitIsClear(owStatus, OW_BUSY)		//���� ��������
#define owIsBusy()		BitIsSet(owStatus, OW_BUSY)			//���� ������
#define OW_DEVICE		6									//����������� ���������� �� ����
#define owDeviceSet()	SetBit(owStatus, OW_DEVICE)			//���������� ���� ����������� ���������� �� ����
#define owDeviceNo()	ClearBit(owStatus, OW_DEVICE)		//��� ����������
#define owDeviceIsSet()	BitIsSet(owStatus, OW_DEVICE)		//��������� ����������� ���������� �� ����
#define owDeviceIsNo()	BitIsClear(owStatus, OW_DEVICE)		//��������� ���������� ���������� �� ����
#define OW_ONLY_SEND	5									//������ �������� ��� ������
#define owSendOnly()	SetBit(owStatus, OW_ONLY_SEND)		//������ �������� �������
#define owSendAndReciv() ClearBit(owStatus, OW_ONLY_SEND)	//�������� � �������
#define owIsSendOnly()	BitIsSet(owStatus, OW_ONLY_SEND)	//����� ��������
#define owIsSndAndRcv()	BitIsClear(owStatus, OW_ONLY_SEND)	//����� � ������ � ��������
#define OW_RECIV_STAT	4									//1 - ������� ����� �������� - ����� �����, 0 - ��������
#define owRecivStat()	SetBit(owStatus, OW_RECIV_STAT)		//������� ��������� � ����� ������ �����
#define owSendStat()	ClearBit(owStatus, OW_RECIV_STAT)	//����� �������� �����
#define owIsRecivStat()	BitIsSet(owStatus, OW_RECIV_STAT)	//���� �����
#define owIsSendStat()	BitIsClear(owStatus, OW_RECIV_STAT)	//���� ��������

void owInit(void);
void owExchage(void);										//������ ����� �� ���� 1-Ware
volatile ONEWARE_EVENT owCompleted;							//������� ���������� �� ��������� ������.
volatile ONEWARE_EVENT owError;								//������� ���������� ��� ������������� ������ �� ����

//------------- ������� ���� 1-Ware
#define ONE_WARE_SEARCH_ROM		0xf0						//����� ������� - ������������ ��� ������������� ��������� ����������� ���������� � ������� ������������ ���������
#define ONE_WARE_READ_ROM		0x33						//������ ������ ���������� - ������������ ��� ����������� ������ ������������� ���������� �� ����
#define ONE_WARE_MATCH_ROM		0x55						//����� ������ - ������������ ��� ��������� � ����������� ������ ���������� �� ������ ������������
#define ONE_WARE_SKIP_ROM		0xcc						//������������ ����� - ������������ ��� ��������� � ������������� ���������� �� ����, ��� ���� ����� ���������� ������������ (����� ���������� � ������������ ����������)

#endif /* ONEWARE_H_ */