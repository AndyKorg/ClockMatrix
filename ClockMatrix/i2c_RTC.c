/*
 * ������ � ������ ���������� ���������� RTC
 */ 

#include "i2c.h"
#include "sensors.h"
#include "ds18b20.h"
#include "bmp180hal.h"
#include "esp8266hal.h"

//volatile u08 TemprCurrent;					//������� �������� ����������� � �������������� ����. ���� RTC � �������� ����������� � ��������� �� 0.25 ��������, ������������ ������ ����� ��������
volatile u08 StatusModeRTC;						//������� ����� ������ � RTC �����������. ������� 5 ��� ��� ������� ����� ������ � RTC
volatile u08 OneSecond;							//������ �������� 0-1 ������ �������

void i2c_RTC_Exchange(void);					//����� � ����������� RTC
SECOND_RTC	GoSecond = Idle,					//������� ���������� ������ ������� �� ���������� RTC
			Go0Second = Idle;					//������� ���������� ������ ������� ������� � ������ ��� �������� ������������ ����������

/************************************************************************/
/* ������ ������ � ���������� RTC, ����� ������ ������������				*/
/* ���������� StatusModeRTC, ��� ������ ��. � ������� i2c_RTC_Exchange  */
/************************************************************************/
void WriteToRTC(void){
	SetModeRTCWrite;
	if (Refresh != NULL)
		SetTimerTask(Refresh, REFRESH_CLOCK*2);//�������� ����� 
}

/************************************************************************/
/* ���������� ������� ���������� ������ ������� �� ���������� �� RTC    */
/************************************************************************/
void SetSecondFunc(SECOND_RTC Func){
	GoSecond = Func;
}
/************************************************************************/
/* ���������� ������ �� ������� ���������� ������ ������� �� ���������� */
/************************************************************************/
SECOND_RTC GetSecondFunc(void){
	return GoSecond;
}

/************************************************************************/
/* ���������� ������� ���������� ������ ������� �������					*/
/************************************************************************/
void Set0SecundFunc(SECOND_RTC Func){
	Go0Second = Func;
}

/************************************************************************/
/* ���������� �� ���������� RTC. ���������� ������ �������              */
/************************************************************************/
ISR(INT_NAME_RTC){
#ifndef INT_SENCE_RTC
	if (BitIsClear(INT_RTCreg, INT_RTC)) return;					//���� ���������� �� �� RTC �� �����.
#endif	
	OneSecond = (OneSecond)?0:1;
	if (GoSecond != Idle)
		(GoSecond)();
	SetTask(espWatchTx);											//�������� ��� ������ ����������� ����� � esp
	if (Watch.Second == 0){											//������������ ������ ������
		if (Go0Second != Idle)										//���� ������� �������� ����������?
			(Go0Second)();
		if ((Watch.Minute & 0xf) == 0){								//������ ����� ������
			SetTask(i2c_ExtTmpr_Read);								//��������� ������� ������ �����������
			SetTask(StartMeasureDS18);								//��������� ������ � ������� 1-Ware
			SetTask(StartMeasuringBMP180);
		}
		for(u08 i=0; i<= SENSOR_MAX; i++)							//��������� ������� ������� ��� ���� ��������
			if (SensorFromIdx(i).SleepPeriod)
				SensorFromIdx(i).SleepPeriod--;
	}
}

/************************************************************************/
/* ����� ������� � RTC                                                  */

/************************************************************************/
/* �������� ������ ���������                                            */
/************************************************************************/
void i2c_OK_RTCFunc(void){
	if (ModeRTCIsWrite){											//���� ������ �� ���������� ����� ������, ��������� ������� ��������
		SetModeRTCRead;
		SetCalcAdd;													//����� ����������� 1 � ��������� �� ���������
	}
	else{															//������� ��������� ����� � ������ ���������
		Watch.Second = i2c_Buffer[clkadrSEC];
		Watch.Minute = i2c_Buffer[clkadrMINUTE];
		Watch.Mode = ClrModeBit(Watch.Mode);
		Watch.Mode |= i2c_Buffer[clkadrHOUR] & (mcModeHourMask | mcAM_PM_Mask);	//������������ ����� ����� � ������� ������ AM/PM
		Watch.Hour =  i2c_Buffer[clkadrHOUR] & ~(ModeIs24(Watch.Mode)? mcModeHourMask : (mcModeHourMask | mcAM_PM_Mask));
		Watch.Date = i2c_Buffer[clkadrDATA];
		Watch.Month = i2c_Buffer[clkadrMONTH] & 0b00011111;			//7-1 ��� ��� ���, ������ ������������
		Watch.Year = i2c_Buffer[clkadrYEAR];
		//�����������
//		if BitIsClear(i2c_Buffer[ctradrSTATUS], RTC_TEMP_BUSY)		//����������� ������
//			TemprCurrent = i2c_Buffer[ctradrMSB_TPR];
	}
	i2c_Do &= i2c_Free;												//����������� ����
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);					//��������� ������ ������ ��������� ���������� �����
}

/************************************************************************/
/* ������ ������			                                            */
/************************************************************************/
void i2c_Err_RTCFunc(void){
	i2c_Do &= i2c_Free;												//����������� ����
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);					//������� ��������� ��������
}

void i2c_RTC_Exchange(void){
	u08 i;

	if (i2c_Do & i2c_Busy){											//���� ������, ���������� �����
		SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);
		return;
	}
	
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;													//������ � ����� � ����
	i2c_Do &= ~i2c_type_msk;										//����� ������
	if (ModeRTCIsWrite){											//------------------ ����� ������
		if ((ClockStatus == csSet)									//��� ������ ��� � ������ ��������� ����� ������ �������
			&&
				(
				(SetStatus == ssDate) || (SetStatus == ssMonth) ||	//���� ������������ ����� ������� - ����, �����, ���
				(SetStatus == ssYear)
				)
			){
			i2c_ByteCount = 5;										//����� ����� ������ �����
			i2c_Buffer[0] = clkadrDAY;								//������� � ��� ������
			i = CalcIsAdd ? AddClock(CurrentCount, SetStatus) : DecClock(CurrentCount, SetStatus);//�������������� ��������� �������� ����� ����
			i2c_Buffer[2] = (SetStatus == ssDate)? i : Watch.Date;
			i2c_Buffer[3] = (SetStatus == ssMonth)? i : Watch.Month;
			i2c_Buffer[4] = (SetStatus == ssYear)? i : Watch.Year;
			i2c_Buffer[1] = WeekRTC(what_day(i2c_Buffer[2], i2c_Buffer[3], i2c_Buffer[4]));//���� ������ ������ ��������������. 1 - ��� ����������� � ���������� RTC, � ������� what_day ���������� 7-��.
		}
		else if (GetWrtAdrRTC == ctradrCTRL){						//------------------ ������ �������� ��������
			i2c_ByteCount = 2;										//������� ����������
			i2c_Buffer[0] = ctradrCTRL;								//����� ���� ������ � RTC
			ClearBit(i2c_Buffer[1], DisableOSC_RTC);				//��������� �������
			SetBit(i2c_Buffer[1], EnableWave_RTC);					//�������� ����� SQW
			ClearBit(i2c_Buffer[1], StartTXO_RTC);					//���������� �����������
			ClearBit(i2c_Buffer[1], RS1_RTC);						//������� ���������� 1 ��
			ClearBit(i2c_Buffer[1], RS2_RTC);
			ClearBit(i2c_Buffer[1], Int_Cntrl_RTC);					//��������� ������ ������ INT � ������ ������ ����������
		}
		else if (GetWrtAdrRTC == ctradrSTATUS){						//------------------ ������ �������� ���������
			i2c_ByteCount = 2;										//������� ����������
			i2c_Buffer[0] = ctradrSTATUS;							//����� ���� ������ � RTC
			SetBit(i2c_Buffer[1], RTC_32KHZ_ENBL);					//�������� ��������� 32 ���
			ClearBit(i2c_Buffer[1], RTC_OSF_FLAG);					//��������� ��� �������, � ������ ������ ����� ������� ����� ���������
		}
		else{														//------------------ ������� ������ ��������� �����, �����, ������
			i2c_ByteCount = 2;										//������� ����������
			i2c_Buffer[0] = GetWrtAdrRTC;							//����� ���� ������ � RTC
			i2c_Buffer[1] = CalcIsAdd ? AddClock(CurrentCount, SetStatus) : DecClock(CurrentCount, SetStatus);	//�������������� ��������� ��������
		}
		i2c_Do |= i2c_sawp;											//������ ������
	}
	else{															//------------------ ������ RTC
		i2c_ByteCount = i2c_MaxBuffer;								//���������� �������� ����
		i2c_PageAddrCount = 1;
		i2c_PageAddrIndex = 0;
		i2c_PageAddress[0] = 0;
		i2c_Do |= i2c_sawsarp;										//����� ������ ������.
	}
	MasterOutFunc = &i2c_OK_RTCFunc;
	ErrorOutFunc = &i2c_Err_RTCFunc;
	
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //���� ��������
	i2c_Do |= i2c_Busy;
}

/************************************************************************/
/* ������ ������ ��������� Watch � RTC                                  */
/************************************************************************/
u08 i2c_RTC_DirectWrt(void){
	if (i2c_Do & i2c_Busy){											//���� ������
		return 0;
	}
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;													//������ � ����� � ����
	i2c_Do &= ~i2c_type_msk;										//����� ������
	i2c_ByteCount = 8;												//����� ����� ��� ��������
	i2c_Buffer[0] = clkadrSEC;										//������� � ������
	i2c_Buffer[1] = Watch.Second;
	i2c_Buffer[2] = Watch.Minute;
	i2c_Buffer[3] = Watch.Hour;										//������ 24-������� ������ �������!
	i2c_Buffer[4] = WeekRTC(what_day(Watch.Date, Watch.Month, Watch.Year));
	i2c_Buffer[5] = Watch.Date;
	i2c_Buffer[6] = Watch.Month;
	i2c_Buffer[7] = Watch.Year;
	i2c_Do |= i2c_sawp;												//������ ������

	MasterOutFunc = &i2c_OK_RTCFunc;
	ErrorOutFunc = &i2c_Err_RTCFunc;

	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //���� ��������
	i2c_Do |= i2c_Busy;
	return 1;
}

/************************************************************************/
/* ������ �������� ������� ����� ��������								*/
/************************************************************************/
void i2c_RTC_StatusReadOk(void){
	if (BitIsSet(i2c_Buffer[ctradrSTATUS], RTC_OSF_FLAG))		//���� ��������� ����������
		RTC_ValidClear();										//�������� ��������� �� ���������
	i2c_Do &= i2c_Free;											//����������� ����
}
void i2c_RTC_StatusReadErr(void){
	RTC_ValidClear();											//�������� ��������� �� ���������
	i2c_Do &= i2c_Free;											//����������� ����
}

/************************************************************************/
/* ������ ������ � RTC                                                  */
/************************************************************************/
void Init_i2cRTC(void){
	//������ �������� ������� ���������� RTC
	while(i2c_Do & i2c_Busy);									//���� ������, ���� ������������
	i2c_SlaveAddress = RTC_ADR;
	i2c_index = 0;												//������ � ����� � ����
	i2c_Do &= ~i2c_type_msk;									//����� ������
	i2c_ByteCount = 1;											//���������� �������� ����
	i2c_PageAddrCount = 1;
	i2c_PageAddrIndex = 0;
	i2c_PageAddress[0] = ctradrSTATUS;							//������ �� �������� �������
	i2c_Do |= i2c_sawsarp;										//����� ������ ������.
	MasterOutFunc = &i2c_RTC_StatusReadOk;
	ErrorOutFunc = &i2c_RTC_StatusReadErr;
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;  //���� ��������
	i2c_Do |= i2c_Busy;
	while(i2c_Do & i2c_Busy);									//��������� ��������� ������
	
	SetWrtAdrRTC(ctradrCTRL);									//��������� RTC. ������ � ������� ��������
	SetModeRTCWrite;
	i2c_RTC_Exchange();											//������ �������� � RTC
	while(ModeRTCIsWrite);										//���� ��������� ������ � RTC
	SetWrtAdrRTC(ctradrSTATUS);									//��������� RTC. ������ � ������� �������
	SetModeRTCWrite;
	i2c_RTC_Exchange();											//������ �������� � RTC
	while(ModeRTCIsWrite);										//���� ��������� ������ � RTC
	Watch.Second = 0;											//���������� �������� �.�. �������� �������� ����� ��������� ������������� ����������, � �������� ��� �� ��������� �� ���������� RTC
	Watch.Minute = 0;
#ifdef INT_SENCE_RTC											//������ � ����������� INTX
	MCUCR |= INT_SENCE_RTC;
#else
	SetBit(PCICR, INT_RTCgrp);
#endif
	SetBit(INT_RTCreg, INT_RTC);								//������ ���������� �� RTC
	SetModeRTCRead;												//����� ������ RTC
	SetTimerTask(i2c_RTC_Exchange, REFRESH_CLOCK);				//������ ������ ��������� ���������� �����
}
