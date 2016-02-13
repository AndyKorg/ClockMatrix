/*
 * ver. 0.1
 */ 

#include "IRMultiHAL.h"
#include "eeprom.h"

#ifndef IR_SAMSUNG_ONLY

#define IR_PULSE_PERIOD		1152000							//����������� ������ ������ ����, 1,152 ms = 1152 us = 1152000
#define IR_RELOAD_PERIOD	19200000						//������������ ������������ ������, us

#define IR_PULSE_THERSHOLD	(IR_PULSE_PERIOD/PERIOD_INT_IR)	//����� ������������ �������� �� �������� ������������ 1 ��� 0
#if (IR_PULSE_THERSHOLD > 65536)
	#error "IR_PULSE_THERSHOLD is greater then 65536"
#endif

#define IR_RELOAD			(IR_RELOAD_PERIOD/PERIOD_INT_IR)
#if (IR_RELOAD > 65536)
	#error "IR_RELOAD is greater 65536"
#endif

#define IR_DELAY_PROTECT	100								//�������� �������� ����� ��-��������

volatile struct ir_t
{
	uint8_t rx_started;										//���� ��������� ��������
	uint32_t rx_buffer;										//����� ������
} Ir;


//��������� ��������� ������ �� ����, ���������� ������������ ��������
ISR(IR_INT_VECT)
{
	if(Ir.rx_started){										//���� �����
		Ir.rx_buffer <<= 1;									//����������� ������ � ������ ������
		if(PeriodIR >= IR_PULSE_THERSHOLD)					//���� ������������ �������� ������ ������ �� ��� ��������� ��������
			Ir.rx_buffer |= 1;
	}
	else
		Ir.rx_started = 1;									//����� ������
	PeriodIR = 0;
}


/************************************************************************/
/* ������� � �� ������ ������. �������� � ��������� ������, ��� ��      */
/* ���������� ����������											    */
/************************************************************************/
void IRReadyCommand(void){
	if (IRReciveReady != NULL){								//���� ������� ��������� �������� �������, �������� ��
		IRReciveReady(REMOT_TV_ADR, Ir.rx_buffer);			//� ������ �������������� ������ ��� ������� � �� ��������� ��������� ��������� �� � ����� �����.
	}
	Ir.rx_buffer = 0;
}

/************************************************************************/
/* ���������� � �������� PERIOD_INT_IR                                  */
/************************************************************************/
void IRMultiInc(void){
	if (Ir.rx_started){
		if (PeriodIR > IR_RELOAD){							//�������� � ������ �� ���� ����� IR_RELOAD, ��� ��������� ������ ������
			StopIRCounting();								//���� ������� �� ��������� ��������� �������
			Ir.rx_started = 0;								
			SetTask(IRReadyCommand);						//��������� ������� �� ���������
		}
		else
			PeriodIR++;
	}
}

/************************************************************************/
/* ������������� ������� ����� ������ � ����� ������                    */
/************************************************************************/
void IRRecivHALInit(void){
	//����������������� ���� �� eeprom. ���� ���� ��� �� ����������������� ���� ��� ������ samsung
	if (EeprmReadIRCode(IR_OK) == EEP_BAD)
		IRcmdList[IR_OK] =			0xAA0800A2;	//Ok			
	if (EeprmReadIRCode(IR_STEP) == EEP_BAD)
		IRcmdList[IR_STEP] =		0x2280882A;	//Menu
	if (EeprmReadIRCode(IR_PLAY_START) == EEP_BAD)
		IRcmdList[IR_PLAY_START] =	0xA80802A2;	//���������������
	if (EeprmReadIRCode(IR_PLAY_STOP) == EEP_BAD)
		IRcmdList[IR_PLAY_STOP] =	0x280882A2; //����
	if (EeprmReadIRCode(IR_INC) == EEP_BAD)
		IRcmdList[IR_INC] =			0x20808A2A; //�����+
	if (EeprmReadIRCode(IR_DEC) == EEP_BAD)
		IRcmdList[IR_DEC] =			0x0080AA2A;	//�����-
	
	Ir.rx_started = 0;										//������� ��� �� ����� ������
	Ir.rx_buffer = 0;
	PeriodIR = 0;
	IRPeriodEvent = IRMultiInc;								//��������� ������� ������������� ������ 30.5 ��
	IntIRUpDownFront();										//����� ��������� ������ �� ���� �������� ����������
	StartIRCounting();										//��������� ���������� - ����� ��������
}

#endif	//#ifndef IR_SAMSUNG_ONLY
