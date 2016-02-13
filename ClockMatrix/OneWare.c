/*
 * OneWare.c
 * ������������ �� ���� 1-Ware � ������� ����������.
 * �� ����� ������ ���� ����������� ������� ���������� ��������� ��� ����� ����� ������� ���������
 * ��� ���������� �� �������. ����� ���������� ���������� OW_REPEAT_END_DELAY()+OW_START_SLOT_DELAY() ���.
 * � ��� ����� ������ ���� �� ���������. 
 * ������� ������� ����� ������������ �������� ���� USART ��� ���� 1-ware.
 * ver 1.2
 */ 

#include "OneWare.h"
#include <util/delay.h>

//------------- �������� ����� 1-Ware
#define OW_DDR					DDRD
#define OW_PORT_OUT				PORTD
#define OW_PORT_IN				PIND
#define OW_PIN					PORTD6
#define OW_INI()				ClearBit(OW_PORT_OUT, OW_PIN)		//�������� ����� 0 ��� �������� � �����
#define OW_NULL()				SetBit(OW_DDR, OW_PIN)				//������� � �����
#define OW_ONE()				ClearBit(OW_DDR, OW_PIN)			//���������
#define OW_IS_NULL()			BitIsClear(OW_PORT_IN, OW_PIN)		//�� ����� ������ �������
#define OW_IS_ONE()				BitIsSet(OW_PORT_IN, OW_PIN)		//�� ����� ������� �������


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
#define OW_DELAY_STOP()			OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//���������� �������
//#define OW_DELAY_STOP()			do {OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0); OW_DISABLE_INT();}while(0)	//���������� �������
#define OW_PRE64()				OW_TCCR = OW_DELAY_INIT | (1<<OW_CS2) | (0<<OW_CS1) | (0<<OW_CS0)	//������������ �� 64
#define OW_PRE32()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (1<<OW_CS0)	//������������ �� 32
#define OW_PRE8()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (1<<OW_CS1) | (0<<OW_CS0)	//������������ �� 8
#define OW_PRE1()				OW_TCCR = OW_DELAY_INIT | (0<<OW_CS2) | (0<<OW_CS1) | (1<<OW_CS0)	//������������ �� 1
#define OW_BORDER				255									//������� �� ������� ���� ����������� ��������
#define OW_DELAY_VALUE(dly, div) (OW_BORDER-(F_CPU/div)/(1000000UL/dly))	//������ �������� �������� ��� ���������� �������� ��� ��������� ������������
#define OW_DELAY_64(delay)	do{OW_PRE64();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 64);}while(0)
#define OW_DELAY_32(delay)	do{OW_PRE32();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 32);}while(0)
#define OW_DELAY_8(delay)	do{OW_PRE8();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 8);}while(0)
#define OW_DELAY_1(delay)	do{OW_PRE1();OW_PRE_RESET();\
								OW_TCNT = OW_DELAY_VALUE(delay, 1);}while(0)
//------------- �������� ��� ����-�����
//������������ �������� Reset �� ���� 1-Ware 480 ��� �������. 
#define OW_RESET_DELAY_us			480
#define OW_RESET_PRE				64
#if (OW_RESET_PRE == 64)
	#define OW_RESET_DELAY()		OW_DELAY_64(OW_RESET_DELAY_us)			
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

//����� ����-�����
#define OW_START_SLOT_DELAY_us		2
#define OW_START_SLOT_DELAY_PRE		1
#if (OW_START_SLOT_DELAY_PRE == 64)
	#define OW_START_SLOT_DELAY()		OW_DELAY_64(OW_START_SLOT_DELAY_us)
#elif (OW_START_SLOT_DELAY_PRE == 32)
	#define OW_START_SLOT_DELAY()		OW_DELAY_32(OW_START_SLOT_DELAY_us)
#elif (OW_START_SLOT_DELAY_PRE == 1)
	#define OW_START_SLOT_DELAY()		OW_DELAY_1(OW_START_SLOT_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_START_SLOT_DELAY_us, OW_START_SLOT_DELAY_PRE) >= 255)	//�������� ����������� �������� �������� ��� ���������� ������������
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_START_SLOT_DELAY_us, OW_START_SLOT_DELAY_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//������������ ����-�����
#define OW_MAX_SLOT_us				60
#define OW_MAX_SLOT_PRE				32
#if (OW_MAX_SLOT_PRE == 64)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_64(OW_MAX_SLOT_us)
#elif (OW_MAX_SLOT_PRE == 32)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_32(OW_MAX_SLOT_us)
#elif (OW_MAX_SLOT_PRE == 1)
	#define OW_MAX_SLOT_DELAY()		OW_DELAY_1(OW_MAX_SLOT_us)
#endif
#if (OW_DELAY_VALUE(OW_MAX_SLOT_us, OW_MAX_SLOT_PRE) >= 255)
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_MAX_SLOT_us, OW_MAX_SLOT_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//���������� ����� ����-�������
#define OW_INTERVAL_SLOT_DELAY_us	10
#define OW_INTERVAL_SLOT_DELAY_PRE	1
#if (OW_INTERVAL_SLOT_DELAY_PRE == 64)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_64(OW_INTERVAL_SLOT_DELAY_us)
#elif (OW_INTERVAL_SLOT_DELAY_PRE == 32)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_32(OW_INTERVAL_SLOT_DELAY_us)
#elif (OW_INTERVAL_SLOT_DELAY_PRE == 1)
	#define OW_INT_SLOT_DELAY()		OW_DELAY_1(OW_INTERVAL_SLOT_DELAY_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_INTERVAL_SLOT_DELAY_us, OW_INTERVAL_SLOT_DELAY_PRE) >= 255)	//�������� ����������� �������� �������� ��� ���������� ������������
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_INTERVAL_SLOT_DELAY_us, OW_INTERVAL_SLOT_DELAY_PRE) <= 2)
	#error "Prescaller less 2"
#endif

//�������� ����� ������ ������ ����-����� ��� ������� ��������� ���� �� ����������
#define OW_REPEAT_END_us			2//9
#define OW_REPEAT_END_PRE			1
#if (OW_REPEAT_END_PRE == 64)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_64(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 32)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_32(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 8)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_8(OW_REPEAT_END_us)
#elif (OW_REPEAT_END_PRE == 1)
	#define OW_REPEAT_END_DELAY()	OW_DELAY_1(OW_REPEAT_END_us)
#else
	#error "Prescaller not valid"
#endif
#if (OW_DELAY_VALUE(OW_REPEAT_END_us, OW_REPEAT_END_PRE) >= 255)
	#error "Prescaller more 255"
#elif (OW_DELAY_VALUE(OW_REPEAT_END_us, OW_REPEAT_END_PRE) <= 2)
	#error "Prescaller less 2"
#endif

#define OW_STEP_2_US				2								//�������� 2 �����������, ���������� ��� ���������� ��������� ����
#define OW_MAX_WAIT_RESET_US		240								//�������� 240 �����������, ������������ ������������ �������� ������ �� ���������� �� ������� reset
#define OW_BIT_IN_BYTE				8								//���������� �������� � �����

//------------ ���������� ������� ------------------------------
void owResetBus(u08 Reset);

typedef void (*OW_EVNT)(u08 Reset);									//��� ��������� �� ������� ��������� ������� ��������� �������
#define OW_DELAY_RESET				1								//�������� ������� ��������� ������� OW_EVNT
#define OW_DELAY_NORMAL				0								//���������� ������ ������� OW_EVNT � ������������ � ���������� ��������� �����
volatile OW_EVNT	EndDelayEvent;									//������� ���������� � ����� ��������

#define ONE_WARE_BUF_LEN			2								//������ ������������ � ����������� ������
volatile u08	owStatus,											//������� ������ ���� 1-Ware, �������� ����������
				owRecivBuf[ONE_WARE_BUF_LEN],						//����� ��� �������� ����
				owSendBuf[ONE_WARE_BUF_LEN];						//����� ��� ������������ ����

/************************************************************************/
/* ����� �����															*/
/************************************************************************/
void owRecivData(u08 Reset){
	static u08 State, ByteCount, BitCount, ByteBuf, TmpGICR;
	
	if (Reset){														//�������� ���������� ��������� ��������
		State = 0;
		ByteCount = 0;
		BitCount = OW_BIT_IN_BYTE;
		ByteBuf = 0;
		return;
	}
	switch (State){
		case 0:														//�������� ����� ����-�����
			OW_NULL();												//������ �������� ������ ����-�����
			TmpGICR = GICR;
			GICR &= ~((1<<INT0) | (1<<INT1) | (1<<INT2));
//			_delay_us(3);
			OW_START_SLOT_DELAY();									//6 us
			State++;
			break;
		case 1:
			OW_ONE();
//			_delay_us(15);											//������� ������ ����� ��������� �������� ������� �� ����������� �������. ������ ����� ������������
			OW_REPEAT_END_DELAY();									//9 us
			State++;
			break;
		case 2:
			BitCount--;
			ByteBuf >>= 1;											//��������� ���
			if OW_IS_ONE()
				SetBit(ByteBuf, 7);
			GICR = TmpGICR;
			OW_MAX_SLOT_DELAY();									//60 us
			if (BitCount == 0){										//���� ������?
				BitCount = OW_BIT_IN_BYTE;
				owRecivBuf[ByteCount] = ByteBuf;					//��������� ��� � ������
				ByteBuf = 0;
				ByteCount++;										//��������� ����
				if (ByteCount == ONE_WARE_BUF_LEN){					//���� ����� ������
					State++;
					break;
				}
			}
			State = 0;												//������ � �������� ������ ����-�����
			break;
		case 3:														//����� ����� ��������, �������������� �������� ����� �������
			OW_INT_SLOT_DELAY();
			State++;
			break;
		case 4:														//����� ��������
			OW_DELAY_STOP();										//������ ���������������
			owFree();												//���� ��������
			owCompleted();											//���������� ������� �� ��������� ��������
			break;
		default:
			break;
	}
}

/************************************************************************/
/* �������� �����														*/
/************************************************************************/
void owSendData(u08 Reset){
	static u08 State, ByteCount, BitCount, ByteBuf;
	
	if (Reset == OW_DELAY_RESET){									//�������� ���������� ��������� ��������
		State = 0;
		ByteCount = 0;
		BitCount = OW_BIT_IN_BYTE;
		ByteBuf = owSendBuf[0];
		return;
	}
	switch (State){
		case 0:														//�������� ����� ����-�����
			OW_NULL();												//������ �������� ������ ����-�����
			OW_START_SLOT_DELAY();									//6 us
			State++;
			break;
		case 1:
//			_delay_us(2);
			if (ByteBuf & 1)										//� ����������� �� ���� ������� ��� ��������� ���� �� 60 ���
				OW_ONE();
			OW_MAX_SLOT_DELAY();									//���� ���� ���� 60 us
			State++;
			break;
		case 2:														//�������� ���������� ����� ����-�������
			OW_ONE();												//��������� ����
			OW_INT_SLOT_DELAY();									//10 us
			BitCount--;
			if (BitCount){											//���� ���� ��� ��������?
				ByteBuf >>= 1;										//��������� ���
			}
			else{													//���� �������, ������� ���������
				ByteCount++;
				BitCount = OW_BIT_IN_BYTE;
				if (ByteCount == ONE_WARE_BUF_LEN){					//���� ����� �������
					if owIsSndAndRcv(){								//���� ��� ��������� ������ �� ����������?
						owRecivStat();								//����������� ������� � ����� ������
						owResetBus(OW_DELAY_RESET);					//����������� ������� ��� ��������� ������ �� ����
						EndDelayEvent = owResetBus;
					}
					else{											//������ �� ��������� ������ ������
						State++;
						break;
					}
				}
				else												//��������� ����
					ByteBuf = owSendBuf[ByteCount];
			}
			State = 0;												//��������� ���
			break;
		case 3:														//����� ��������
			OW_INT_SLOT_DELAY();									//10 us
			State++;
			break;
		case 4:														//����� ��������
			OW_DELAY_STOP();										//������ ���������������
			owFree();												//���� ��������
			owCompleted();											//���������� ������� �� ��������� ��������
			break;
		default:
			break;
	}
}

/************************************************************************/
/* ����� ���� � �������� ������ �� ��������� �� ����                    */
/************************************************************************/
void owResetBus(u08 Reset){
	static u08 State;
	
	if (Reset == OW_DELAY_RESET){									//�������� ���������� ��������� ��������
		State = 0;
		return;
	}
	if (State == 0){												//��������� �������� ������, ������ ��������� ���������
		OW_ONE();													//��������� ����
		OW_REPEAT_END_DELAY();
	}
	else if((State >=1) && (State<=(OW_MAX_WAIT_RESET_US/OW_STEP_2_US))){ //�������� ���� �� ��������� ������� ������ � �������������� 2 ��� � ������� 240 ���
		OW_INT_SLOT_DELAY();										//���� �������� ������� ������������� ��������� �� ������� ��������, �� ������� ���� ���.
		if OW_IS_NULL(){											//����� ���������, ����� ���������� �������
			owDeviceSet();											//���������� ����������
			OW_RESET_DELAY();										//�������� ��� 480 ��� ��� ��������� �������� PRESENCE
			State = OW_MAX_WAIT_RESET_US/OW_STEP_2_US;				//���������� ����� ����
		}
	}
	else{															//�������� ������ �� ���������� ���������, ��������� � ��������� �����������
		if owDeviceIsNo(){											//���� ���������� �� ���������� �� ����
			OW_DELAY_STOP();										//��� ���������� ����������� ���� � �������� ������� ������
			ClearBit(TIMSK, OW_INTERRAPT);							//��������� ���������� �� �������
			owFree();												//���������� ����
			owError();												//������� ������� ������
			return;
		}
		EndDelayEvent = owIsRecivStat()? owRecivData : owSendData;	//� ����������� �� ������ ������ ����� �������� ��������������� ���������
		EndDelayEvent(OW_DELAY_RESET);								//����������� ������ �������
		OW_INT_SLOT_DELAY();										//������ ���������� �������� ����� �������� ����������
	}
	State++;
}

ISR(OW_DELAY_INT){
	EndDelayEvent(OW_DELAY_NORMAL);
}

/************************************************************************/
/* ������ ����� �� ���� 1-Ware											*/
/************************************************************************/
void owExchage(void){
	if owIsBusy(){													//���� ���� ������ �� ������ ��������� � ����
		owError();
		return;
	}

	ClearBit(OW_DDR, OW_PIN);										//���� �� ���� - ��� �� ������ ������
	SetBit(OW_PORT_OUT, OW_PIN);									//�������� ������������� ��������

	EndDelayEvent = owResetBus;
	EndDelayEvent(OW_DELAY_RESET);									//����������� ������ ������� ������� ��������� ��������
	owBusy();														//������ ����
	owDeviceNo();													//������� ��� ���������� ���� ���
	owSendStat();													//����� ��������

	OW_NULL();														//������ �����-��������
	ClearBit(OW_PORT_OUT, OW_PIN);									//������� ���� � �����. ��� �� �� ��������� ��� 0, �� ��� �� ������ ������
	
	OW_RESET_DELAY();
	OW_ENABLE_INT();												//��������� ����������
}
