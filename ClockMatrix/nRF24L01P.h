/*
 * ������ � ������� nRF24L01P ����� hard-spi
 * ����� ������ � �������� ������.
 * ��������� ���� �����������. ���� ��� ������� ����������� �� ����������� SENSOR_NO = (0xc0)
 * ��������� �������� ����� ����� �� ������ ��������� � ����������, �� ������������ ����������� ��������.
 * � ������ ����� ����� ���������� ��� ����������� ������������ ���, � �� ������ ��������������.
 * ����� ������� ���� �������� ����������� ����� ��� ������������ ������������.
 * � ����� ���������� ������������ ������� ���������. ������������ ������� ���������� � ���� ����
 * ����. ������ ���� ��� �������� ����������� ��� ������� WDT
 * ������ � ������ ����� ��� ������� ������������ ������� WDT. ������� ���� �������� ���������� ������.
 * ������ ���� ������:
 * 		BufTx[0] = ATTINY_13A_8S_SLEEP;		//������� ��� ������� ���, ������� �� 8 ������
 *		BufTx[1] = 0x00;					//������� ���� �������� ���
 *		BufTx[2] = 0x01;					//������� ���� ��������
 *		nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | 0b00000000, BUF_LEN, BufTx);	//�������� ����� ��� ������ 0
 */ 


#ifndef NRF24L01P_H_
#define NRF24L01P_H_

#define nRF_PIPE				5				//����� ������������� ������
#define nRF_SEND_LEN			4				//����� ������������� ������ ������. ������ ��������� �� ��������� nRF_SEND_LEN � ����� ������������ ����� ....\ClockMatrix\nRF24L01P.h
#define nRF_TMPR_ATTNY13_SENSOR	0x01			//������ ����������� �� Attiny13. ������ ��������� �� ��������� nRF_TMPR_ATTNY13_SENSOR � ����� ������������ ����� ....\ClockMatrix\nRF24L01P.h
#define nRF_ACK_LEN				3				//����� ������ �� ����� ��� ������ ������ �� ������������. ������ ��������� �� ��������� nRF_ACK_LEN � ����� ������������ ����� ....\ClockMatrix\nRF24L01P.h
#define nRF_RESERVED_BYTE		0				//���� ����������������� ��� �������� �������������
#define nRF_SENSOR_NO			0xfa00			//������� �� ���� 1-Ware ���.	������ ��������� �� ��������� SENSOR_NO � ����� ������������ ����� ...\TempSensorDSandNRF24\nRF24L01P.h


#if ((nRF_ACK_LEN<1) && (nRF_ACK_LEN>32))
	#error "Incorrect value nRF_ACK_LEN. The nRF_ACK_LEN should be between 1 to 32"
#endif

#include <avr/io.h>
#include "Clock.h"
#include "avrlibtypes.h"
#include "nRF24L01P Reg.h"
#include "bits_macros.h"

#define nRF_DDR					DDRC			//SPI � nRF24L01+
#define nRF_PORTIN				PINC
#define nRF_PORT				PORTC
#define nRF_MOSI				PINC2
#define nRF_MISO				PINC5
#define nRF_SCK					PINC4
#define nRF_CSN					PINC3			//����� ���������
//#define nRF_CE					PINC2			//������ ��������. ���� ����� CE ��������� �������� � �������� �� ���������������
//#define nRF_IRQ					PINC4			//���������� �� ������

#define nRF_SELECT()			ClearBit(nRF_PORT, nRF_CSN)
#define nRF_DESELECT()			SetBit(nRF_PORT, nRF_CSN)

#ifdef nRF_CE
#define nRF_GO()				SetBit(nRF_PORT, nRF_CE)	//���� ����������������
#define nRF_STOP()				ClearBit(nRF_PORT, nRF_CE)	//����
#else
#define nRF_GO()				//���� ���������� ���������� ���������� ��������� �� ������������ �� ������� �� �����
#define nRF_STOP()				
#endif

#ifdef nRF_IRQ
#define nRF_IS_RECIV()			BitIsClear(nRF_PORTIN, nRF_IRQ)	//�����-�� �������� ���������
#define nRF_NO_RECIV()			BitIsSet(nRF_PORTIN, nRF_IRQ)	//������ ���
#endif

//��������� ����������� WDT ��� ATTINY13A
#define ATTINY_13A_WDP0 0
#define ATTINY_13A_WDP1 1
#define ATTINY_13A_WDP2 2
#define ATTINY_13A_WDP3 5
#define ATTINY_13A_16MS_SLEEP	((0<<ATTINY_13A_WDP3) | (0<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	//������������ ������ ����� 16 ��
#define ATTINY_13A_05S_SLEEP	((0<<ATTINY_13A_WDP3) | (1<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	// -/- 0,5 ������
#define ATTINY_13A_1S_SLEEP		((0<<ATTINY_13A_WDP3) | (1<<ATTINY_13A_WDP2) | (1<ATTINY_13A_WDP1) | (0<<ATTINY_13A_WDP0))	// -/- 1 �������
#define ATTINY_13A_8S_SLEEP		((1<<ATTINY_13A_WDP3) | (0<<ATTINY_13A_WDP2) | (0<ATTINY_13A_WDP1) | (1<<ATTINY_13A_WDP0))	// -/- 8 ������

#define nRF_WDT_INTERVAL_S		8				//�������� ���������� WDT
#define nRF_MEASURE_NORM_MIN	10				//������ ��������� ����������� ������, � �������. ����� WDT ������ ���� ATTINY_13A_8S_SLEEP
#define nRF_MEASURE_TEST_SEC	1				//������ ��������� � �������� ������ � ��������. ����� WDT ������ ���� ATTINY_13A_1S_SLEEP
#define nRF_INTERVAL_NORM		((u16)(nRF_MEASURE_NORM_MIN*60)/nRF_WDT_INTERVAL_S)
#define nRF_INTERVAL_NORM_LSB	((u08)nRF_INTERVAL_NORM & 0xff)			//������� ����
#define nRF_INTERVAL_NORM_MSB	((u08)(nRF_INTERVAL_NORM>>8) & 0xff)	//������� ����

#define nRF_PERIOD_TEST			100				//������ �������� ��������� ������ ������-�����������

void nRF_Init(void);
void nRF_StartRecive(void);						//�������� ������ �� �����

#endif /* NRF24L01P_H_ */