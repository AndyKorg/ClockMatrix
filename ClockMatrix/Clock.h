/*
 * Clock.h
 * ��������� �������� ��� ���������� �����, �������� ��������� � ����������
 */ 

#ifndef CLOCK_H_
#define CLOCK_H_

#include <avr/io.h>
#include "avrlibtypes.h"

//############################ �������� ��������� ��� ���������� �����  #######################################
#ifndef F_CPU
#define F_CPU 20000000UL							//���� �� ���������� ������� � �������� ������� ���������� ���
#endif

//############################ ������� ########################################################################
//------- ��� �������� ��������
//#define SHIFT_REG_TYPE_HC595						//�������� ��� ��������� ���� 74HC595
#define SHIFT_REG_TYPE_MBI50XX						//�������� ��� ��������� MBI
//------- ��� ����������
#define LED_MATRIX_COMMON_COLUMN					//����� ������� - �� ���� ����������� ������������ ��������� �������
//#define LED_MATRIX_COMMON_ROW						//����� ������ - �� ���� ����������� ������������ ��������� ������

//------- ���������� ������ SPI �������
#define DISP_DDR_SPI		DDRA					//���� SPI
#define DISP_PORT_SPI		PORTA
#define DISP_SPI_SCK		PORTA1					//��������
#define DISP_SPI_MOSI		PORTA0					//����� ������
#define DISP_SPI_SS			PORTA6					//����� �������
#define clckLowLED			ClearBit(DISP_PORT_SPI, DISP_SPI_SCK)
#define clckHightLED		SetBit(DISP_PORT_SPI, DISP_SPI_SCK)	//������ ���� � ������� �� �������� low->hight
#define upldLowLED			ClearBit(DISP_PORT_SPI, DISP_SPI_SS)
#define upldHightLED		SetBit(DISP_PORT_SPI, DISP_SPI_SS)	//����� ������ �� ��������� �� �������� �� �������� low->hight
//------- �������������� ���������� ��������
#define DISP_OFF_DDR		DDRD					//���� ���������� ���������-���������� �������
#define DISP_OFF_PORT		PORTD
#define DISP_OFF_PIN		PORTD7
#define DisplayOff			SetBit(DISP_OFF_PORT, DISP_OFF_PIN)
#define DisplayOn			ClearBit(DISP_OFF_PORT, DISP_OFF_PIN)
#ifdef SHIFT_REG_TYPE_HC595
	#define DISP_CLR_DDR	DDRA					//���� ������� �������� �������
	#define DISP_CLR_PORT	PORTA
	#define DISP_CLR_PIN	PORTA4
	#define DisplayClrReg	ClearBit(DISP_CLR_PORT, DISP_CLR_PIN)
	#define DisplayNoClrReg	SetBit(DISP_CLR_PORT, DISP_CLR_PIN)
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	#define DISP_SDO_DDR	DDRA					//���� �� ������� �������� ����������� �� MBI5039
	#define DISP_SDO_PORT	PORTA
	#define DISP_SDO_PIN	PORTA4
#else
	#error "Shift register undefined"
#endif
//------- ����������� ������� � ����������� �� ������������
#ifdef LED_MATRIX_COMMON_COLUMN
	#define DISP_STEP_OUT	3						//���������� �������� � ���������� 32 ��� ��� ������ ���������� ��������� ������ �� �������
#elif defined(LED_MATRIX_COMMON_ROW)
	#define DISP_STEP_OUT	12						//���������� �������� � ���������� 32 ��� ��� ������ ���������� ��������� ������ �� �������
#else
	#error "Undefined display type"
#endif
#define DELAY_DISP			64						//�������� ����� ������� ��������. �� ������ ���� ������� �.�. ���������� �������� �������.
#define DISP_BOUNDARY_LVL	12						//������� ������������ �� ������� ����������� ������� ������������ �� ����� �������
#define DELAY_INTERVAL		2						//�������� ���������� ������� ����� ����������� �������� DISP_BOUNDARY_LVL
#define ILLUM_PORT		(0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0) //���������� ���� ���������� ������������, ������ ADC2
#define PrescalerADC	(1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) //���������� ��� �� ��� ����� ������� ������� �������, �.�. �������� �������������� �� �����
//------- ��������� �������
#define DISP_ROW			8						//���������� ����� � �������
#define DISP_COL_DOT		8						//���������� ����� � �������
#define DISP_COL			24						//���������� �������� � �������
#define DISP_DIGIT			(DISP_COL/DISP_COL_DOT)	//���������� ����������� ��������


//############################ ����������� ���������  ########################################################################
#define VOLUME_IS_DIGIT								//��������������� ������ ���� �� ������������ �������� ��������� ���������

//############################ ��������� SPI for SD-CARD  ####################################################################
//---------  ����������� ��� ����������
#define SD_SPI_BUILD_IN								//��������������� ���� ������������ ����������� SPI, � �� ����������
//---------  ��� ��������������� ������� 5�->3.3�
//#define SD_SPI_DIRECT								//��������������� ���� ������������ ��������������� ������ ��� �������������� 5� � 3.3�
//---------  ����� SPI
#define SD_PORT_DDR			DDRB					//���� ��� ���������� SPI
#define SD_PORT_OUT			PORTB
#define SD_PORT_IN			PINB
#define SD_PORT_CS_DDR		DDRB					//���� ��� ������ CS
#define SD_PORT_CS_OUT		PORTB
#define SD_PORT_CS_IN		PINB
//---------  Pins SPI
#define SD_CS				PORTB4					//��� ����������� SPI ����� �������� ��� �������� �� SS �����������, �.�. ���������� �������� ��� ���������������� ����� SPI
#ifdef SD_SPI_BUILD_IN
#define SD_DI				PORTB5					//���������� SPI
#define SD_DO				PORTB6
#define SD_CLK				PORTB7
#else
#define SD_DI				PORTB5					//����������� SPI
#define SD_DO				PORTB6
#define SD_CLK				PORTB7
#endif
//---------  ������� SD-card
#define SD_PORT_CASEDDR		DDRD					//���� ��� ������� ������� �������� � ������� ������
#define SD_PORT_CASEIN		PIND
#define SD_INS				PORTD5					//������� �������� � �����
#define SD_WP				PORTB1					//������ ������ ������ - � ����� �� ������������, �������� ��� �������������
#define SD_TEST_PERIOD		100						//������ �������� ������� SD-����� � �����, ��
#define SD_WHITE_PERIOD		300						//�������� ����� �������������� ��������
#define NoSDCard()			((SD_PORT_CASEIN & (1<<SD_INS)) != 0x00)	//��� �������� � �����
#define SD_CARD_ATTEMPT_MAX	10						//������������ ���������� ������� �������������� ��-�����

//############################ ����������  ########################################################################
//#define EXTERNAL_INTERAPTS_USE					//������������ ������� ���������� ��� ������ ������

//------------- �������� ������
#ifdef EXTERNAL_INTERAPTS_USE
	#define KEY_STEP_INT		INT0				//��� ����� ���������� ������ Step
	#define KEY_STEP_INTname	INT0_vect			//��� ���������� ��� STEP
	#define KEY_OK_INT			INT1				//������ ��
	#define KEY_OK_INTname		INT1_vect			//��� ����������
	#define KEY_INT_MASK		(Bit(KEY_STEP_INT) | Bit(KEY_OK_INT))
#else
	#define KEY_PORTIN_STEP		PIND				//���� ������ STEP
	#define	KEY_PORTIN_OK		PINA				//���� ������ OK
#endif

#define KEY_STEP_PORT			PIND4				//����� ����� ������
#define KEY_OK_PORT				PINA5

#define KEY_STEP_FLAG			0					//��� ������ STEP � ���������� ��������� ������
#define KEY_OK_FLAG				1					//��� ������ ��

#ifndef EXTERNAL_INTERAPTS_USE
	#define SCAN_PERIOD_KEY		150					//������ ������������ ������ ���� �� ������������ ����������
#endif

#define PROTECT_PRD_KEY			30					//�������� �������� ��� ���������� �������� ���������, ��

//############################ �������� ��������� ��������� ########################################################################
#define VOLUME_DDR_CS_SHDWM	DDRB					//���� ��� ������ � ����������� ��� ������� CS � SHDWM
#define VOLUME_PRT_CS_SHDWM	PORTB
#define VOLUME_SHDWN		PORTB1					//���� ������ ���������� ��������� �������
#define VOLUME_CS			PORTB0					//���� ������ CS
#define VOLUME_DDR_CLK_SI	DDRC					//���� ��� ������ � ����������� ��� ������� CLK � SI
#define VOLUME_PRT_CLK_SI	PORTC
#define VOLUME_CLK			PORTC7					//���� ������ CLK
#define VOLUME_SI			PORTC6					//���� ������ ������ SI

//############################ ���������� ������� ########################################################################
// ������ 0 - ������������ ����������� ��� ����� �������� � Sound.c, ������� �� OC0
// ������ 1 - ������������ ����������� ��� ����� �������� � Sound.c, �������� �������
// ������ 2 - ������������ ��� ������� ���������� �� ���� 1-ware, �������� � OneWare.c

//############################ ������� ���������� ########################################################################
#define IR_INT				INT0					//���������� �� ������� ����� �� ��������
#define DISP_INT_VECTOR_32HZ INT1_vect				//������ ���������� �� ���������� 32768 ��
// INT2 - ������������ ��� ������� ������������� ���������� (��� INT_NAME_RTC), ������ � i2c.h
//------------- ������������ ���������� �� RTC �� ��������� ����. ��������������� ������� ���� ���� ������������ ���������� PCINT � �������� ���� ������������ INTX
#define INT_SENCE_RTC		Bit(ISC2)				//������������ �� ������������ ������
#ifdef INT_SENCE_RTC
	// --- ��� ������ � ����������� INT
	#define INT_NAME_RTC	INT2_vect				//��� ���������� �� RTC
	#define INT_RTCreg		GICR					//������� ����� ����������
	#define	INT_RTC			INT2					//����� ����������
#else
	// --- ��� ������ � ����������� PCINT
	#define INT_RTCgrp		PCIE2					//������ ���������� �� RTC
	#define INT_NAME_RTC	PCINT2_vect				//��� ���������� �� RTC
	#define INT_RTCreg		PCMSK2					//������� ����� ����������
	#define	INT_RTC			PCINT18					//����� ����������
#endif

//############################ ��������� � ���� I2C ������ � IIC_ultimate.h  ########################################################################

//############################ ��-��������  #########################################################################################################
//#define IR_SAMSUNG_ONLY					//��������������� ���� ������������ ������������ �����. � ���� ������ �� ����� ��� �������� ������� ����� ����������
#if (IR_INT == INT0)
	#define IR_INT_BIT0		ISC00
	#define IR_INT_BIT1		ISC01
	#define IR_INT_VECT		INT0_vect
#elif (IR_INT == INT1)
	#define IR_INT_BIT0		ISC10
	#define IR_INT_BIT1		ISC11
	#define IR_INT_VECT		INT1_vect
#else
	#error "IR interrupt no defined!"
#endif

#define IntiRDownFront()	do {SetBit(MCUCR, IR_INT_BIT1); ClearBit(MCUCR, IR_INT_BIT0);} while (0)	//��������� ���������� IR �� ��������� �����
#define IntIRUpFront()		do {SetBit(MCUCR, IR_INT_BIT1); SetBit(MCUCR, IR_INT_BIT0);} while (0)		//����������� �����
#define IntIRUpDownFront()	do {ClearBit(MCUCR, IR_INT_BIT1); SetBit(MCUCR, IR_INT_BIT0);} while (0)	//� ����������� ����� � ��������� �������� ����������
#define StartIRCounting()	SetBit(GICR, IR_INT)	//������ ������
#define StopIRCounting()	ClearBit(GICR, IR_INT)	//���������� ����� ���������

//############################ �������� ������� - ���� �� ������������ ########################################################################
// TODO: �������� �������� �������
/*
#define POWER_ENBL_INT	PCINT8						//��� ���������� ���������� ���������� �������

#define POWER_INTgrp	PCIE1						//������ ���������� ��� �������� ���������� ������� ����
#define POWER_INTname	PCINT1_vect					//��� ���������� -\\- ��� ���������� ��������� � keyboard.c
#define POWER_INTreg	PCMSK1						//��� �������� ��� ����� ���������� -\\-
#define POWER_PORTin	PINC						//���� �����������
*/


//----------------- ����� �������� ��������� ��� ���������� ����� ---------------------------------------------------------------

//############################ ��������� ��������� ����� ########################################################################

//---------  ������� ������ ��� (�������)
#define EACH_HOUR_START		9						//��� � �������� ���������� ��� �������� ���� �� ��������
#define EACH_HOUR_STOP		21						//��� �� �������� ��� ������������ ���� �� ��������

//---------  ������ � ����������
#define INET_TIME_DAY		6						//������� - ����� ��� ������ � ������� ����� ��������� ������ ����� �� �����
#define INET_TIME_HOUR		0x10					//10 ����� ���� - ��� ��� ������� ������� �������. � ������� BCD � 24-� ������� ������� TODO: �������� ��� ��������� 12-� �������� �������
#define INET_TiME_MINUTE	0x10					//10 ����� ������ ��� ������� ������� �������. � ������� BCD

//---------  ������
#define SECOND_DAY_SHOW		15						//�� ����� ������� �������� ������� ������ � ����� � ������ ������ �������� �������

#define CENTURY_DEF			0x20					//����� ����

#define TIMEOUT_RET_CLOCK_MODE_MIN	10				//������� �������� � �������� ����� ������ �����, � �������, �� ��������� � ��������� �������.


//############################ ����� ��������� �������� ����� ########################################################################


//############################ ����������� ###########################################################################################
typedef void (*VOID_PTR_VOID)(void);				//��������� �� �������, ��� ���������� ������ �������

//---------  ���������� ��������� ����� � �����������
enum tClockStatus									//������ �����
{
	csNone,											//������ �����
	csClock,										//������������ ������� �����
	csSecond,										//������ �������
	csDate,											//������������ ����, �����, ���� ������
	csSet,											//��������� �������
	csAlarmSet,										//��������� �����������
	csAlarm,										//��������� ��������
	csTune,											//��������� ���������� �����
	csTempr,										//����� �����������
	csSensorSet,									//��������� ��������
	csSDCardDetect,									//���������� ��������� SD-����� � �����, ��������� ������� ���������
#ifndef IR_SAMSUNG_ONLY
	csIRCodeSet,									//��������� ����� ��-������
#endif	
	csInternetErr,									//����� ������ ������ ��� ��������� � esp8266
	csPowerOff										//���������� � ����� ����������� �������� �������
};

enum tSetStatus{									//�������� ������ ���������
	ssNone,											//���������� ����� ��� ���������� ����� ����
	ssSecond,										//��������� ������ - ��������� ������� �� UP ���������� ������� � ����
	ssMinute,										//��������� �����
	ssHour,											//�����
	ssDate,											//����. ��� ������ ��������� ����������� �� �������� ���� � ��� ���� ��������� ���������
	ssMonth,										//�����
	ssYear,											//���
	ssNumAlarm,										//����� ����������
	ssAlarmSw,										//���������-���������� ����������
	ssAlarmDelay,									//������������ ����������
	ssAlarmMondy,									//������ ���������� � �����������
	ssAlarmTuesd,									//������ ���������� � �������
	ssAlarmWedn,									//������ ���������� � �����
	ssAlarmThur,									//������ ���������� � �������
	ssAlarmFrd,										//������ ���������� � �������
	ssAlarmSat,										//������ ���������� � �������
	ssAlarmSun,										//������ ���������� � �����������
	ssTuneSound,									//���������� ����� ����� ������ � ���� ���������� ��������
	ssEvryHour,										//������ ������ ���
	ssKeyBeep,										//������ �� ������� ������
#ifdef VOLUME_IS_DIGIT
	ssVolumeBeep,									//��������� ����� ������� ������
	ssVolumeEachHour,								// -/- ���������� �������
	ssVolumeAlarm,									// -/- ����������
	ssVolumeTypeAlarm,								//��� ����������� ���������
#endif
	ssFontPreTune,									//��������������� ����� ��� ��������� ������
	ssFontTune,										//��������� �����
	ssHZSpeedTune,									//�������� �������������� ���������
	ssHZSpeedTSet,									//��������� ��������
	ssSensNext,										//����� �������
	ssSensSwitch,									//���������-���������� �������
	ssSensPreAdr,									//��������� � ������ ������ �������
	ssSensAdr,										//����� ������ �������
	ssSensPreName,									//��������� � ����� �����
	ssSensName1,									//����� ������ ����� �����
	ssSensName2,									//����� ������ ����� �����
	ssSensName3,									//����� ������� ����� �����
	ssSensWaite,									//�������� ����������� �������
#ifndef IR_SAMSUNG_ONLY								//���� ������������ ������������ �� �����
	ssIRCode,										//������������ ������� ��� ������ ������ ��
#endif
	ssSDCardNo,										//SD-����� �����������
	ssSDCardOk,										//SD-����� ���������� � ����������������
	ssTimeSet,										//������ � ������ ������� �� ���������
	ssTimeLoaded,									//����� �����������
	ssTuneNet,										//��������� ����
	ssIP_Show										//����� ������ IP ��� ������ station
};

struct sClockValue{									//�������� ������� ��� ����� � �����������. � BCD-�������.
	u08	Second;										//������� ����� ����������� � �������� REFRESH_CLOCK.
	u08	Minute;
	u08	Hour;
	//u08 Day;										//���� ������ � ��������� �������, 1 - �����������, 2 - ����������� � �.�.
	u08 Date;										//���� ������
	u08 Month;									
	u08	Year;										//�� 0 �� 99, ������ 21 ���.
	u08 Mode;										//����� ������ - ��. ����
};

extern volatile struct sClockValue Watch;			//������� �������� �����. ��������� ���� Watch.Mode ������ ������� �� �� ������������ ��� �������� ���������� � ��������� �����������

//----------------- ���� ���������� Mode � ��������� sClockValue.
//---------  ��������� �������� ������ � eeprom. � �������� �������� ���������� ������������ ������ Watch.Mode
#define epSTATE_BIT			7						//0-������� ��������, 1 - �����
#define epWriteIsBusy()		BitIsSet(Watch.Mode, epSTATE_BIT)	//������� ������ �����
#define epWriteIsFree()		BitIsClear(Watch.Mode, epSTATE_BIT)	//������� ��������
#define epWriteBusy()		SetBit(Watch.Mode, epSTATE_BIT)		//������ ������� ������
#define epWriteFree()		ClearBit(Watch.Mode, epSTATE_BIT)	//���������� �������
//---------  ������ ������� ���������� �����.
#define mcModeHourBit		6						//����� ���� ������ ������� ���������� �����, ��������! ��������� � ������� ���� � RTC!
#define mcModeHourMask		Bit(mcModeHourBit)		//����� ����� ������ �� ������� ��� �� �������
#define SetMode12(Mc)		SetBit(Mc, mcModeHourBit)//���������� ����� 12-� �������
#define SetMode24(Mc)		ClearBit(Mc, mcModeHourBit)	//����� 24 ����
#define ModeIs12(Mc)		BitIsSet(Mc, mcModeHourBit)
#define ModeIs24(Mc)		BitIsClear(Mc, mcModeHourBit)

#define mcAM_PM_Bit			5						//����� ���� AM/PM, ��������! ��������� � ������� ���� � RTC!
#define mcAM_PM_Mask		Bit(mcAM_PM_Bit)		//����� ����� ������ �� ������� ��� �� �������
#define SetAM(Mc)			ClearBit(Mc, mcAM_PM_Bit)//���������� ����� AM
#define SetPM(Mc)			SetBit(Mc, mcAM_PM_Bit)	//���������� ����� PM
#define ClockIsAM(Mc)		BitIsClear(Mc, mcAM_PM_Bit)
#define ClockIsPM(Mc)		BitIsSet(Mc, mcAM_PM_Bit)
#define ClrModeBit(mc)		(mc & ~(mcModeHourMask | mcAM_PM_Mask))	//����� ������� ����� � AM/PM
//---------  ���� ����������� ������ esp8266
#define espInstallBit		4						//������ esp8266 ����������
#define espModuleSet()		SetBit(Watch.Mode, espInstallBit)
#define espModuleRemove()	ClearBit(Watch.Mode, espInstallBit)
#define espModuleIsSet()	BitIsSet(Watch.Mode, espInstallBit)
#define	espModuleIsNot()	BitIsClear(Watch.Mode, espInstallBit)
//---------  ��������� RTC ����� ������ ��
#define RTC_NotValidBit		3						//���� ���������� ���������� RTC
#define RTC_IsValid()		BitIsClear(Watch.Mode, RTC_NotValidBit)	//RTC ���������
#define RTC_IsNotValid()	BitIsSet(Watch.Mode, RTC_NotValidBit)	//��� ���� ������� � RTC �����������
#define RTC_ValidSet()		ClearBit(Watch.Mode, RTC_NotValidBit)	//���������� ���� ������������ RTC
#define RTC_ValidClear()	SetBit(Watch.Mode, RTC_NotValidBit)		//�������� ���� ������������ RTC
//��������� ����
//2
//1
//0

//---------  ���������� ��������� � ����
void SensSetNext(void);								//����� ���������� ������� ��� �����������
void SensAdrSet(void);								//����� ������ �������
void SensNameSet(void);								//������������ ����� � ����� �������
void SensSwitch(void);								//����������-���������� �������� �������
u08 SensTestON(void);								//�������� ��������� �������.

//---------  ��������� ����� ������ ������������� ������ ��
#ifndef IR_SAMSUNG_ONLY
void IRCodeSave(void);								//�������� ��� �������
u08 IRNextCode(void);								//��������� ������ � ������� ����� ������ ��. ���������� 1 ���� ��� ������� ���� ���������
#endif

//############################ ����������� ���������� � ������� ###########################################################################################
#define MULTIx8(c) ((c)<<3)							//������ �������� ��������� �� 8. ���������� � �������������!

extern volatile struct sClockValue *CurrentCount;	//������� ������� �����. ����� ���� � �������� ������, � ������ ����������
extern struct sAlarm *CurrentShowAlarm;				//������� ������������ ���������
extern u08	CurrentAlarmDayShow,					//������� ���� ������ ��� �������� ����������
			CurrentSensorShow;						//������� ������������� ������

extern volatile enum tClockStatus ClockStatus;		//��������� �����
extern volatile enum tSetStatus SetStatus;			//�������� � ������ ���������

void SetClockStatus(volatile enum tClockStatus Val, enum tSetStatus SetVal);//��������� ������ ������ �����

void StartDateShow(void);							//�������� ���� ���� ���
void ShowDate(void);								//������������ ������ ���� � ���� � ��������

VOID_PTR_VOID Refresh;								//������ �� ������� ����������� ����� ������� ����� ��������
	
#endif /* CLOCK_H_ */