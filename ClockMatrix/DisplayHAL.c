/*
 * ����� ������ ������� ������ ������ ������ �� ���������.
 * ver. 1.6
 */ 
//TODO:���������� �������� �������� ������ ���������� � MBI5039
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stddef.h>
#include "avrlibtypes.h"
#include "bits_macros.h"
#include "Clock.h"
#include "DisplayHAL.h"
#include "EERTOS.h"
#ifdef IR_SAMSUNG_ONLY
	#include "IRreciveHAL.h"
#endif

volatile u08 Pallor = 0;								//��������� ����. 0 - ����� �����, �������������� ��� ������� ������������

u08 DisplayBuf[DISP_COL];								//����� ������������� ������

//����� ����� �� ������� ��������
void OutByteSPI(u08 Value){
	u08 i;

	for (i=0; i<8; i++){
		SetBitVal(DISP_PORT_SPI, DISP_SPI_MOSI, (Value & 1)?1:0);//������� ���
		Value >>= 1;
		clckLowLED;										//����������� ����� CLK
		clckHightLED;
		clckLowLED;
	}
}

/************************************************************************/
/* �������� ��� ���� ��� �� ���������� ������ �������� ��������� �����. */
/* �������� ��� ���������� ������ ���������								*/
/************************************************************************/
inline u08 DelayForBrightness(u08 *Value){
	
	if (IRPeriodEvent != NULL)							//���������� ������� ��� �� ������� ���� ��� ����
		IRPeriodEvent();
	
	if ((*Value) != 0){											
		(*Value)--;
		if ((*Value) <= Pallor)							//��������� �������
			DisplayOff;
		else
			DisplayOn;
		return 1;
	}
	return 0;
}

/************************************************************************/
/* ��������� ������ RTOS                                                */
/************************************************************************/
#define DelayRTOS	33									//�������� ��� ��������� 1 �� ����������
void TimeServiceRTOS(void){
	static u08 CountTime = DelayRTOS;
	
	if (CountTime)
		CountTime--;
	else{
		CountTime = DelayRTOS;
		TimerService();
	}
}
	
/************************************************************************/
/* ����� �� �������� �������. ������������								*/
/* ���������� �� 32 ���													*/
/************************************************************************/
#if defined(LED_MATRIX_COMMON_ROW)

//��� ������ � �������
ISR(DISP_INT_VECTOR_32HZ){

	static u08
		Delay = DELAY_DISP,
#if defined(SHIFT_REG_TYPE_HC595)
#error "not ready yet"
#elif defined(SHIFT_REG_TYPE_MBI50XX)
		row1=0, row2=0, row3=0,
#endif
		ComRow = 0,
		Status = 0;
		
	TimeServiceRTOS();									//��������� ������ RTOS
	
	if (DelayForBrightness(&Delay))						//�������� ����������?
		return;
	
	if (Status == 0)									//��� 0. ������ ���������� �����
		row1=0,row2=0,row3=0;
	else if ((Status>=1) && (Status<=8)){				//���� � 1 �� 8 ��� ���������� �����
		row1 <<= 1;
		row2 <<= 1;
		row3 <<= 1;
		if BitIsSet(DisplayBuf[Status-1], ComRow) row1 |= 1;
		if BitIsSet(DisplayBuf[Status-1+8], ComRow) row2 |= 1;
		if BitIsSet(DisplayBuf[Status-1+16], ComRow) row3 |= 1;
	}
#if defined(SHIFT_REG_TYPE_MBI50XX)						//���� ������ ��� MBI50XX
	else if (Status == 9)								//��� 9. ��������� ��������������� ������
		OutByteSPI(Bit(ComRow));
	else if (Status == 10)								//��� 10. ��������� ������ � ���������� 1
		OutByteSPI(row1);
	else if (Status == 11)								//��� 11. ��������� ������ � ���������� 2
		OutByteSPI(row2);
	else if (Status == DISP_STEP_OUT){					//��� 12. ��������� ������ � ���������� 3 � ���������� ��������� �����������
		OutByteSPI(row3);
		upldHightLED;
		upldLowLED;
		ComRow++;										//���������� ���������� �����
		if (ComRow == DISP_ROW)
			ComRow = 0;
		Delay = DELAY_DISP-DISP_STEP_OUT;
		Status = 0xff;
	}
#endif
	Status++;
}
#elif defined(LED_MATRIX_COMMON_COLUMN)

//��� ������ � ��������
ISR(DISP_INT_VECTOR_32HZ){

	static u08
		Delay = DELAY_DISP,
		ComCol = 0,										//������� ����� �������
		Status = 0;

	TimeServiceRTOS();									//��������� ������ RTOS

	if (DelayForBrightness(&Delay))						//�������� ����������?
		return;
	
	#define ByteOfCol(Nm)	(DisplayBuf[(DISP_COL-1)-(ComCol+(8*Nm))])	//���� ������ ������� ��� ���������� Nm � ������� Col
#ifdef SHIFT_REG_TYPE_HC595								//��� ����� �� ��������� HC595
	#define DispByte1	ByteOfCol(2)					//���� 1 ��� ����� ���� ������� ���������
	#define DispByte2	ByteOfCol(1)					//���� 2 ��� ����� ���� ������� ���������
	#define DispByte3	ByteOfCol(0)					//���� 3 ��� ����� ���� ������� ���������
	#define DispByte4	(~Bit(ComCol))					//���� 4 ��� ����� ���� ������� ���������
#elif defined(SHIFT_REG_TYPE_MBI50XX)					//��� ����� �� MBI50XX
	#define DispByte1	Bit(ComCol)						//���� 1 ��� ����� ���� ������� ���������
	#define DispByte2	ByteOfCol(2)					//���� 2 ��� ����� ���� ������� ���������
	#define DispByte3	ByteOfCol(1)					//���� 3 ��� ����� ���� ������� ���������
	#define DispByte4	ByteOfCol(0)					//���� 4 ��� ����� ���� ������� ���������
#else
	#error "Undefined shift register type"
#endif

	if (Status == 0){
#ifdef SHIFT_REG_TYPE_HC595								//��� ����� �� ��������� HC595
		upldLowLED;										//����������� ����� �������� � �������
#endif
		OutByteSPI(DispByte1);							//������� ���� 1
	}
	if (Status == 1)
		OutByteSPI(DispByte2);
	if (Status == 2)
		OutByteSPI(DispByte3);
	if (Status == DISP_STEP_OUT){
		OutByteSPI(DispByte4);
#ifdef SHIFT_REG_TYPE_HC595								//��� ����� �� ��������� HC595
		upldHightLED;									//����������� ������ ���������
#elif defined(SHIFT_REG_TYPE_MBI50XX)					//��� ����� �� MBI50XX
		upldHightLED;
		upldLowLED;
#else
	#error "Undefined shift register type"
#endif
		Status = 0xff;
		Delay = DELAY_DISP-DISP_STEP_OUT;
		ComCol++;										//��������� �������
		if (ComCol == DISP_COL_DOT)						//��� ������� ��������. �������� � ������
			ComCol = 0;
	}													//������� �������������� ����������� �� ���������
	Status++;											//��������� ��� ���������� �������
}
#else
#error "Shift register undefined"
#endif

/************************************************************************/
/* ��������� ������������ ����� � ����������� �������                   */
/************************************************************************/
void IllumMeasure(void){
	ADMUX = (0<<REFS1) | (1<<REFS0) |		//������� ���������� ����� ���������� �������
			 ILLUM_PORT |					//�������� ������������ �� ����� �������������
			 (1<<ADLAR);					//������������ ���������� �����
	ADCSRA = (1<<ADEN)	|					//��������� ���
			 (1<<ADSC)  |					//����� ��������������
			 (1<<ADIE)	|					//��������� ���������� �� ��������� ��������������
			 (0<<ADATE) |					//��������� ���������
			PrescalerADC;
	SetTimerTask(IllumMeasure, 1000);		//��������� ������������ ����� 1 �������
}

/************************************************************************/
/* ��������� ��������� ������������										*/
/************************************************************************/
ISR(ADC_vect){
	u08 i = (u08)(ADC>>8);

#if (DISP_BOUNDARY_LVL >= (DELAY_DISP/DELAY_INTERVAL))
#error "The boundary of brightness control shall be less than" (DELAY_DISP/DELAY_INTERVAL)
#endif
	
	if (ClockStatus == csClock){			//����������� ������� ��������� ������ � ������ ����������� �����������
		if (i<=DISP_BOUNDARY_LVL)
			Pallor = DELAY_DISP-i-(DISP_STEP_OUT+3);//���������� DISP_STEP_OUT ��� ����������� ���������� ������� ������� ��� ������� �� ��� �� �������� �������
		else if (i<=(DELAY_DISP/DELAY_INTERVAL))
			Pallor = DELAY_DISP-(i*DELAY_INTERVAL);
		else
			Pallor = 0;
	}
	else
		Pallor = 0;
}

/************************************************************************/
/* ������� ��������� ������                                             */
/************************************************************************/
void ClearScreen(void){
	u08 i;
	
	for (i=0;i<=DISP_COL-1;i++)
	DisplayBuf[i] = 0;
}

void DispRefreshIni(void)
{
	DISP_DDR_SPI |= (1<<DISP_SPI_SCK) | (1<<DISP_SPI_MOSI) | (1<<DISP_SPI_SS);
	DISP_OFF_DDR |= (1<<DISP_OFF_PIN);
	DisplayOn;								//�������� ����� �� ������� 

#ifdef SHIFT_REG_TYPE_HC595
	DISP_CLR_DDR |= (1<<DISP_CLR_PIN);
	DisplayClrReg;							//�������� �������� TPIC
	DisplayNoClrReg;
	upldHightLED;
	clckHightLED;
	upldLowLED;								//����������� ����� �������� � �������
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	clckLowLED;								//����������� ����� CLK
#else
#error "Shift register undefined"
#endif

	OutByteSPI(0);
	OutByteSPI(0);
	OutByteSPI(0);
	OutByteSPI(0);
#ifdef SHIFT_REG_TYPE_HC595
	upldHightLED;							//������� �������������� ����������� �� ���������
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	upldHightLED;
	upldLowLED;
#else
#error "Shift register undefined"
#endif
	ClearScreen();

	MCUCR |= (1<<ISC11) | (1<<ISC10);		//������������� ���������� INT1 �� ������ ��������
	GICR |= (1<<INT1);						//��������� ���������� �� INT1
	
	IllumMeasure();							//������ ���������� ������������
}
