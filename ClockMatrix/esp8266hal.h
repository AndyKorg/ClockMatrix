/*
 * �������� ������� � esp8266
 * ��. ..\Web_base\app\include\customer_uart.h
 */ 


#ifndef ESP8266HAL_H_
#define ESP8266HAL_H_

#define ICACHE_FLASH_ATTR
#include "C:\Espressif\Web_base\app\user\Include\clock_web.h"

#include "avrlibtypes.h"
#include "Volume.h"

#define ESP_WHITE_START	10000									//�������� ����� ������������� ������

// ������� ��� ����� ������������ ����� uart
#define CLK_CMD_TYPE	0b11000000								//���� ���� �������
#define CLK_CMD_WRITE	0b10000000								//������� ������
#define CLK_CMD_TEST	0b11000000								//������� ������������
#define ClkWrite(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_WRITE)	//������ � ���� ��� ������
#define ClkTest(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_TEST)	//������� ������������
#define ClkRead(cmd)	(cmd & (~CLK_CMD_WRITE))				//��������� �� ����� ��� ������
#define ClkIsRead(cmd)	((cmd & CLK_CMD_TYPE) == 0)				//������� ������
#define ClkIsWrite(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_WRITE)	//������� ������
#define ClkIsTest(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_TEST)	//������� ������������
#define ClkCmdCode(cmd)	(cmd & (~CLK_CMD_TYPE))					//��� ������� ��� ����

#define CLK_NODATA		0										//��������� ���������� ������ � �������. ������� ��� ��������

extern u08* espStationIP;										//����� IP � ���� ������ XXX.XXX.XXX.XXX

void StartGetTimeInternet(void);								//��������� ������� ������� ������� �� ���������
u08 espInstalled(void);											//���������� 1 ���� ������ ����������
void espInit(void);												//�������� ������� ������ esp
void espUartTx(u08 Cmd, u08* Value, u08 Len);					//�������� ������� �� ������
void espWatchTx(void);											//�������� � ������ ������� �����
void espVolumeTx(enum tVolumeType Type);						//�������� � ������ ����� ������ �������� � ���������
void espSendSensor(u08 numSensor);								//�������� � ������ �������� ������� NumSensor
void espGetIPStation(void);										//������ ��������� IP ������ ������ � ������ station
void espNetNameSet(void);										//����������� � ���� �� ����� � ����� � sd �����

#endif /* ESP8266HAL_H_ */