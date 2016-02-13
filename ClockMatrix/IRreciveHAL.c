/*
 *	������������� ������ � ��-���������
 * �������� ��������� ��� ������ Samsung http://www.techdesign.be/projects/011/011_waves.htm � http://rusticengineering.com/2011/02/09/infrared-room-control-with-samsung-ir-protocol/
 * ������������ ���������� INT0 - ��� ����������� ������� � ������ ��������� � INT1 (��������� � ��������) ��� ������� 30,5 �� ����������
 * ����� ���� �������� �� ������ ��������� ������ �� ��������� �� ����������
 * ���� �� ������ ������������ ��� ��������� �������� �� ������� �������� ����������� � ��������.
 * ver 1.2
 */ 

#include "IRreciveHAL.h"
#include "EERTOS.h"

#ifdef IR_SAMSUNG_ONLY		//<<<<<<<<-------------------------------- ���������� ���� �� ������������ �������� Samsung
/*
 *	���� ������ ������ samsung AA59-00793A �� ���������� Samsung Smart-TV
 */
#define REMOT_PLAY_START	0xe2										//������ ��������������� (������ ���������������)
#define REMOT_PLAY_STOP		0x62										//���������� ��������������� (������ ����)
#define REMOT_MENU_STEP		0x58										//������ ������ STEP �� ���������� (������ MENU)
#define REMOT_MENU_OK		0xf2										//������ OK �� ���������� (������ GUIDE)
#define REMOT_CHANAL_UP		0x48										//��������� ����� ������ - ����� ��������� ����� � ��������
#define REMOT_CHANAL_DOWN	0x08										//�������� ��������

#define REMOT_0				0x88										//0
#define REMOT_1				0x20										//1
#define REMOT_2				0xa0										//2
#define REMOT_3				0x60										//3
#define REMOT_4				0x10										//4
#define REMOT_5				0x90										//5
#define REMOT_6				0x50										//6
#define REMOT_7				0x30										//7
#define REMOT_8				0xb0										//8
#define REMOT_9				0x70										//9

//------------------ ������� ��������� Samsung, ns. � ������������ ��� �� �������� ��������
#define StartBitPeriodIR	((u08) (4500000/PERIOD_INT_IR))	//������ ������� ������������ ����� ����, 4.5 �� = 4500 000 ns
#define BeginBit0PeriodIR	((u08) (560000/PERIOD_INT_IR))	//������ ������� ������������ ������ ���� � ����� ���� �� ��������� ���� � ����-����, 0,56 �� = 560 000 ns
#define EndBit1PeriodIR		((u08) (1690000/PERIOD_INT_IR))	//������ ������� ������������ ����� ���� �� ��������� �������, 1,69 ��
#define Period4500IsCorrect	((PeriodIR >= (StartBitPeriodIR-2)) && (PeriodIR <= (StartBitPeriodIR+3))) //������ �������� � �������� 4500 ���
#define Period560IsCorrect	((PeriodIR >= (BeginBit0PeriodIR-2)) && (PeriodIR <= (BeginBit0PeriodIR+3))) //������ �������� � �������� 560 ���
#define Period1690IsCorrect	((PeriodIR >= (EndBit1PeriodIR-2)) && (PeriodIR <= (EndBit1PeriodIR+3))) //������ �������� � �������� 1690 ���

#define PeriodMinimum		(560000/PERIOD_INT_IR/2)		//����������� ������������ ��������� ��� �������

volatile u08  StatusIR;										//��������� ���������
			 
//----------------- �������� ���������� StatusIR
#define StatIRStartBit1		0								//����������� ��������� ������ ����������
#define StatIRStartBit2		1								//����������� c�������� ������ ����������
#define StatIRStartBitOK	2								//������ ���� ����� ��������
#define StatIRBeginBit		3								//���������� ������-����� ���� ������
#define StatIREndBit		4								//���������� �������� ��������� ���� ������

volatile u08 CommandIR, AdresIR;							//�������� ����� � ������

/************************************************************************/
/* �������������� ���� �������� ������� �����							*/
/* ���������� 10 ���� ��� �� ������������� �����						*/
/************************************************************************/
u08 ConvertIRCodeToDec(u08 Code){
	switch (Code){
		case REMOT_0:								//������ ����, ������������ ������ ��� ��������� ��������� � ������ ��������� ����� ��� �����������
			return 0;
			break;
		case REMOT_1:
			return 1;
			break;
		case REMOT_2:
			return 2;
			break;
		case REMOT_3:
			return 3;
			break;
		case REMOT_4:
			return 4;
			break;
		case REMOT_5:
			return 5;
			break;
		case REMOT_6:
			return 6;
			break;
		case REMOT_7:
			return 7;
			break;
		case REMOT_8:
			return 8;
			break;
		case REMOT_9:
			return 9;
			break;
		default:
			return 10;
			break;
	}
}

/************************************************************************/
/* ����� �������� ����������� �������                                   */
/************************************************************************/
inline void ResetIR(void){
IntiRDownFront();											//������������ ��������� �����
StatusIR = StatIRStartBit1;									//��������� ����� ��������
}

/************************************************************************/
/* ������� ���������� � �������� PERIOD_INT_IR                          */
/************************************************************************/
void IRPeriodAdd(void){
	PeriodIR++;
}

/************************************************************************/
/* ������������� ��������� ��                                           */
/************************************************************************/
void IRRecivHALInit(void){

IRcmdList[IR_OK] = REMOT_MENU_OK;							//��������� ������ ������ ������ ��� ������ samsung
IRcmdList[IR_STEP] = REMOT_MENU_STEP;
IRcmdList[IR_PLAY_START] = REMOT_PLAY_START;
IRcmdList[IR_PLAY_STOP] = REMOT_PLAY_STOP;
IRcmdList[IR_INC] = REMOT_CHANAL_UP;
IRcmdList[IR_DEC] = REMOT_CHANAL_DOWN;
IRPeriodEvent = IRPeriodAdd;
ResetIR();
StartIRCounting();											//����� �������� ������� �� �� ���������
}

/************************************************************************/
/* ��������� ��������� ���� �� �� ������                                */
/************************************************************************/
void IRReadyCommand(void){
	if (IRReciveReady != NULL)								//���� ������� ��������� �������� �������, �������� ��
		IRReciveReady(AdresIR, CommandIR);
}

/************************************************************************/
/* ����������� ������� � ������ ��������� �� ��-���������               */
/************************************************************************/
ISR(IR_INT_VECT){
	static u08 RecivByte, /*CommandIR, AdresIR, */NumBit, NumByte;
	
	if ((StatusIR != StatIRStartBit1) && (PeriodIR <= PeriodMinimum))	//������������ ������� �������� �������� ����� ������ ������ ���������� ��������
		return;
		
	switch (StatusIR){
		case StatIRStartBit1:								//��������� ��������� ����� �������� ���������
			PeriodIR = 0;
			StatusIR = StatIRStartBit2;						//���� ����������� ������������ ������
			IntIRUpFront();
			break;
		case StatIRStartBit2:								//����������� ����� ������� ����������� ����� ����
			if Period4500IsCorrect{							//������������ ����� ������ � ����������� ����� ������������ ����������� ����� ����?
				PeriodIR = 0;
				IntiRDownFront();
				StatusIR = StatIRStartBitOK;				//���� ��������� ������� ����������� ��������� � ������ ������
			}
			else
				ResetIR();
			break;
		case StatIRStartBitOK:								//�������� ������ ���� ����� ���
			if Period4500IsCorrect{							//������������ ����� ����������� � ������ ����� ������������ ����������� ����� ����?
				PeriodIR = 0;
				IntIRUpFront();
				StatusIR = StatIREndBit;					//��������� ����� ��������� ������ ���� ������
				RecivByte = 0;
				AdresIR = 0;
				NumBit = 0;
				NumByte = 0;
			}
			else
				ResetIR();
			break;
		case StatIRBeginBit:								//������ ���� ������ ��������� ���� ������
			IntIRUpFront();
			StatusIR = StatIREndBit;						//��������� ���
			if Period560IsCorrect{							//����� �������� ��������� ������ ������� �������� ����, ���������� ���
				RecivByte = (RecivByte<<1) & 0xfe;
			}
			else if Period1690IsCorrect
			{												//������ �������� ��������� ������ ������� ���������� ����, ���������� ���
				RecivByte = (RecivByte<<1) | 0x1;
			}
			else{											//�� �� ������������, �����-�� ������
				ResetIR();
				return;
			}
			NumBit++;
			if (NumBit == 8){								//������ ����?
				switch (NumByte){
					case 0:									//������ ������ �����
						AdresIR = RecivByte;
						break;
					case 1:									//������ ���� ����� ���������
						if (RecivByte != AdresIR){			//������ ������?
							ResetIR();
							return;
						}
						break;
					case 2:									//������ ������ ���� ������
						CommandIR = RecivByte;
						break;
					case 3:									//������ ��������� ���� ������, ���������
						if ((RecivByte & CommandIR) == 0){	//��� ���������, ������� ������� ���������
							StopIRCounting();				//��������� ����� ������ ���� �� ����� ���������� �������
							SetTask(IRReadyCommand);		//������ � ������� ������� �������
						}
						ResetIR();
						return;
					default:
						break;
				}
				NumByte++;									//��������� ����
				NumBit = 0;
			}
			PeriodIR = 0;
			break;
		case StatIREndBit:									//������ ������ ��������� ������ ���� ������
			if Period560IsCorrect{							//������ ���� ������������� �������� 
				PeriodIR = 0;
				IntiRDownFront();
				StatusIR = StatIRBeginBit;					//�������� ����� ��������� ����� ����. ��� ������ � ���������� ��� ����������� - ������� ��� ����
			}
			else											//�� �� ������������, �����-�� ������
				ResetIR();
		default:
			break;
	}
}

#endif	//IR_SAMSUNG_ONLY
