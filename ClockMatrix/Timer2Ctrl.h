/*
 * ���������� �������� 2 ��� ��������� �����. ��������������, ��� ���������� ��������� �������� �������� ��������� ��
 * ������������ ���������� ������������ ��������� � OW_DELAY_INT
 * ����� �������: ������� ���� ��������� �������. �� ������� � �� ������� ����� ������ ������������ � 
 * ������������ ������-���� ����������.
 * � Mega ������ 2 ����� ��������� ������������, ������ ��� � ����
 */ 


#ifndef TIMER2CTRL_H_
#define TIMER2CTRL_H_

#include <avr/io.h>
#include "avrlibtypes.h"

//------------- ��������� �������
#define OW_TCNT					TCNT2								//��� �������
#define OW_TCCR					TCCR2
#ifndef PSR2
#define PSR2					1									//������-�� ���� ��� �� ������ � io.h
#endif
#define OW_PRE_RESET()			SetBit(SFIOR, PSR2)					//����� ������������ ������ 2-��� ��������, ������ ���� ������������ ����������� ������ ���� ������ ����� 2
#define OW_ENABLE_INT()			SetBit(TIMSK, TOIE2)				//��������� ���������� �� ������������
#define OW_DISABLE_INT()		ClearBit(TIMSK, TOIE2)				//��������� ���������� �� ������������
#define OW_DELAY_INT			TIMER2_OVF_vect						//������ �������������� ������ ��������
#define OW_INTERRAPT			TOIE2								//���� ���������� ����������
#define OW_DELAY_INIT			(0<<FOC2) | (0<<WGM20) | (0<<WGM21) | (0<<COM21) | (0<<COM20) //��� ����� ������� �������� �������� ����� ������!
#define OW_CS0					CS20								//���� ���������� ������������
#define OW_CS1					CS21
#define OW_CS2					CS22
//------------- ������� ���������� �������������, ������ ������������� ������ ����
#define OW_DELAY_STOP()			OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//���������� �������
#define OW_PRE64()				OW_TCCR = OW_DELAY_INIT | (1<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//������������ �� 64
#define OW_PRE32()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (1<<OW_CS0)	//������������ �� 32
#define OW_PRE1()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (1<<OW_CS0)	//������������ �� 1
#define OW_BORDER				255									//������� �� ������� ���� ����������� ��������
#define OW_DELAY_VALUE(dly, div) (OW_BORDER-(F_CPU/div)/(1000000UL/dly))	//������ �������� �������� ��� ���������� �������� ��� ��������� ������������
#define OW_DELAY_64(delay)	do{OW_PRE64();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 64);}while(0)
#define OW_DELAY_32(delay)	do{OW_PRE32();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 32);}while(0)
#define OW_DELAY_1(delay)	do{OW_PRE1();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 1);}while(0)
							
//------------ ���������� ������� ���������� ------------------------------
typedef void (*TM_OWR_EVNT)(u08 Reset);								//��� ��������� �� ������� ��������� ������� ��������� �������
#define FUNC_DELAY_RESET			1								//�������� ������� ��������� ������� TM_OWR_EVNT
#define FUNC_DELAY_NORMAL			0								//���������� ������ ������� TM_OWR_EVNT � ������������ � ���������� ��������� �����
extern volatile TM_OWR_EVNT			EndDelayEvent;					//������ �� ������� ���������� � ����� ��������. ����� ��� ������� ��������������� ��� ������ � ��������

extern volatile u08 TimerFlag;										//��������� �������� �������������� ������

//------------ �������� ������ ��������� �������� ------------------------------
#define TM_BUSY_BIT					0								//���� ��������� �������, ��������� ������� ��������� ��� ���������� ��� ������ ������ � ��������
#define TM_FREE_BIT1				1
#define TM_FREE_BIT2				2
#define TM_FREE_BIT3				3
#define TM_FREE_BIT4				4
#define TM_FREE_BIT5				5
#define TM_FREE_BIT6				6
#define TM_FREE_BIT7				7

#define TM_BUSY()					SetBit(TimerFlag, TM_BUSY_BIT)
#define TM_FREE()					ClearBit(TimerFlag, TM_BUSY_BIT)
#define TM_IS_BUSY()				BitIsSet(TimerFlag, TM_BUSY_BIT)
#define TM_IS_FREE()				BitIsClear(TimerFlag, TM_BUSY_BIT)

//------------- ������ ���������� �������� ��� �������
/*
#define OW_RESET_DELAY_us			480								//��������� ������������ - 480 ���
#define OW_RESET_PRE				64								//�������� ������������ - ���������� �������. 
																	//������ ���� ��� ����� ������, ��� �� �� ����� ���������� ��� ����� ������ ������. 
																	//� ������ ������� �������� ���������� ������������� �������� ������� - 255 ��������. ���� ����� ������ �� ������ ������� � ����������
#if (OW_RESET_PRE == 64)
	#define OW_RESET_DELAY()		OW_DELAY_64(OW_RESET_DELAY_us)	//OW_RESET_DELAY() - ���� ������� �������� ������������ � ���� ��������� ��� ������ �������.
#elif (OW_RESET_PRE == 32)
	#define OW_RESET_DELAY()		OW_DELAY_32(OW_RESET_DELAY_us)
#elif (OW_RESET_PRE == 1)
	#define OW_RESET_DELAY()		OW_DELAY_1(OW_RESET_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) >= 255)		//�������� ����������� �������� �������� ��� ���������� ������������
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_RESET_DELAY_us, OW_RESET_PRE) <= 2)
	#error "Prescaller less 2"
#endif
*/

#endif /* TIMER2CTRL_H_ */