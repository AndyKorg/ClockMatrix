/*
 * �������� �������� ����� � EEPROM
 * ��������! ��-�� ����������� ������ ��� ���������� ��������� ���������� ��������� ����� ������ ������������ ���������!
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>
#include "eeprom.h"
#include "bits_macros.h"
#include "EERTOS.h"
#include "i2c.h"
#include "Sound.h"
#include "Display.h"
#include "sensors.h"
#include "IRrecive.h"
#ifdef VOLUME_IS_DIGIT
#include "Volume.h"
#endif

typedef u08 (*EP_EVNT)(u08* Data);											//��� ��������� �� ������� ��������� ����� ������. ���������� � Data ���� ������� ���������� �������� � ���������� 0 ���� Data ������������� � 1 ���� ���������� Data ����������
#define EP_NORMAL				0											//���������� ������ ������� EP_EVNT � ������������ � ���������� ��������� �����
#define EP_END					1											//������ �������� ���������, ���������� Data ����������� � �� ������� ������
volatile EP_EVNT				epEndEvent;									//������� ���������� ����� ������ ������ �����

#define epWriteStop()			ClearBit(EECR, EERIE);						//��������� ���������� �� EEPROM, � ������������� � ��������� ������
#define epWriteStart()			SetBit(EECR, EERIE);						//��������� ���������� �� ����� ������ - ����� ������

#define epAlarmSize				(sizeof(struct sAlarm))						//������ ������� eeprom ��� ������ ����������
#define epSensSize				(sizeof(SensorFromIdx(0).Name)+sizeof(SensorFromIdx(0).Adr)+sizeof(SensorFromIdx(0).State)) //������ eeprom ��� ������ �������
#define EEPROM_SENSOR_ADR		0											//����� ����� ������ ������� � ������
#define EEPROM_SENSOR_STATE		1											//����� ����� ���������

u16 epAdr;																	//����� ������ ������. ���� ����� EEPROM_NULL �� ����� �� ���������
volatile u16 epAdrWrite;													//������� ����� ������ � eeprom

#define EEPROM_SIGNATURE		0xaa										//��������� � EEPROM �������������
#define EEPROM_NO_SIGNATURE		0x55										//��������� � EEPROM �� �������������, ���������� ��������� ��������� �����

#define EEPROM_BASE				0											//������� ����� � �������� ���������� �������� � eeprom
#define EEPROM_NULL				(E2END+1)									//������� �������������� ������ eeprom

//����� ��������� �����
#define EEPROM_TUNE_COUNT		4											//���������� ���� ��� �������� ��������. ������ ���� ��� ����� ������ ����. ������ ������ �����. ������ � ��������� �������� ������� ������
#define EEPROM_TUNE_FONT		0											//���� 0 ��� ������ ������
#define EEPROM_TUNE_FLAG		1											//��������� �����
#define EEPROM_TUNE_HZ_SPEED1	2											//�������� ������� ������ - ������� ����
#define EEPROM_TUNE_HZ_SPEED2	3											//�������� ������� ������ - ������� 

//���� �������� ����� ������ �� ������ ����� �������� ��������
#define EEPROM_TUNE_EACH_HOUR	7											//����� ���� ��������� ��������
#define EEPROM_TUNE_KEY_BEEP	6											//���� ������
//5 ��������� ���� �����
//4
//3
//2
//1
//0

//���� ������ ��
#define epIrCodeSize			(IR_NUM_COMMAND*IR_NUM_BYTS)				//������ ������� ��� �������� ����� ������ �� ������
#if (epIrCodeSize > 256)
	#warning "epIrCodeSize is big!"
#endif

//��������� ���������� ���������
#ifdef VOLUME_IS_DIGIT
#define EEPROM_TUNE_EACH_VOL	0											//���� ������ ��������� ��������
#define EEPROM_TUNE_KEY_VOL		1											//��������� ������
#define EEPROM_TUNE_ALARM_VOL	2											//��������� ����������
#define EEPROM_TUNE_ARARM_TYPE_VOL	3										//����� ����������� ��������� ����������
#define EEPROM_TUNE_ADD_BYTE	(EEPROM_TUNE_ARARM_TYPE_VOL+1)				//���������� �������������� ���� ��� �������� ������ � ������ ���������
	#if ((EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_ALARM_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_KEY_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_KEY_VOL) || (EEPROM_TUNE_ARARM_TYPE_VOL < EEPROM_TUNE_EACH_VOL))
		#error "EEPROM_TUNE_ARARM_TYPE_VOL to be the largest number of!"
	#endif
#else
#define EEPROM_TUNE_ADD_BYTE	0											//��� ��������� ���������� �� ������������
#endif

//��������! ��-�� ����������� ������ ��� ���������� ��������� ���������� ��������� ����� ������ ������������ ���������!
#define epPacketSize			((epAlarmSize*ALARM_MAX)+(epSensSize*SENSOR_MAX)+EEPROM_TUNE_COUNT+epIrCodeSize+EEPROM_TUNE_ADD_BYTE+1)	//������ ������. ����� 1 ��� ����� ��� ��������� ������������ ������
#define epAlarmOffset			(epAdr+1)									//�������� ��� �������� ���������� �� ������ ������
#define epSensorOffset			(epAlarmOffset+(epAlarmSize*ALARM_MAX))		//��� ���c����
#define epIrCodeOffset			(epSensorOffset+(epSensSize*SENSOR_MAX))	//��� ����� ������ ��
#define epTuneOffset			(epIrCodeOffset+epIrCodeSize)				//��� �������� ����� � �������� ���������

//-------- ����� �������������� ������
#define eepromWriteBusy()		BitIsSet(EECR, EEWE)						//������ ������ �������
#define eepromWriteIsFree()		BitIsClear(EECR, EEWE)						//������ �� ������ ��������� ������

volatile u08 epStepWr, epCountWr;											//�������� ����� � �������� ���������� ������ ����� � �������� ������. 
																			//������� ����� ���������� ������� ��� ���� ������� ������� �� ���������� ��������� ������

/************************************************************************/
/* ������ ����� �� eeprom									            */
/************************************************************************/
u08 EeprmRead(volatile const u16 Adr){
	while(eepromWriteBusy());												//�������� ���������� ���������� �������� � EEPROM
	EEAR = Adr;
	SetBit(EECR, EERE);
	return EEDR;
}

/************************************************************************/
/* ������������� ������ eeprom - ����������� ������� ������������       */
/************************************************************************/
void EepromInit(void){
	u08	Sig;

	epAdr = EEPROM_BASE;
	do{																		//����� ���������� ������ ������ �������
		Sig = EeprmRead(epAdr);
		epAdr += epPacketSize;												//��������� ����� ������
	} while ((Sig == EEPROM_NO_SIGNATURE) && (epAdr < (E2END-epPacketSize)));
	if (Sig == EEPROM_SIGNATURE)											//���� ������ ��������� �� ����� eeprom ��������� �� ���, � ��������� ������ NULL
		epAdr -= epPacketSize;												//���������� ����� ������ ����� � ������
	else
		epAdr = EEPROM_NULL;
	epWriteFree();															//����� �������� ���������� ���������
}

/************************************************************************/
/* �������� ���������� ������ � ������ EEPROM.							*/
/* ���� ������ ������ ��������� ������ �� ���� ��������� �����			*/
/* ����������															*/
/* ���������� EEP_READY ���� ����� ��������� �������� � �������,		*/
/*	EEP_BAD - ���														*/
/************************************************************************/
u08 EepromReady(void){
	u08 White = 0xff;

	if (epAdr != EEPROM_NULL){												//������ ���������
		while (epWriteIsBusy()){											//������� ������ � eeprom ��� �����
			White--;
			if (White == 0)
				return EEP_BAD;												//������� ����� �����
		}
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* ������ �������� ���������� � ������� ������ Alrm						*/
/* ���������� 1 ���� ��������� ������ � 0 ���� ��������� �� �������		*/
/************************************************************************/
u08 EeprmReadAlarm(struct sAlarm* Alrm){
	
	u08 Buf[epAlarmSize];

	if (EepromReady()){
		for(u08 Count=0; Count<epAlarmSize; Count++)						//��������� ��������� �� eeprom � �����
			Buf[Count] = EeprmRead((epAlarmOffset+(epAlarmSize*(Alrm->Id)))+Count);
		memcpy(Alrm, &Buf, epAlarmSize);									//��������� ��������� ���������� �� ������
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* ������ �������� ��������												*/
/* ���������� 1 ���� ��������� ������ � 0 ���� ��������� �� �������		*/
/************************************************************************/
u08 EeprmReadSensor(void){
	
	u08 Buf[epSensSize];

	if (EepromReady() == 0)													//������ �� ������ � ������
		return EEP_BAD;
	for(u08 SensCount = 0; SensCount<SENSOR_MAX; SensCount++){
		for(u08 Count=0; Count<epSensSize; Count++)							//��������� ������ �� eeprom � �����
			Buf[Count] = EeprmRead(epSensorOffset+epSensSize*SensCount+Count);
		SensorFromIdx(SensCount).Adr = Buf[EEPROM_SENSOR_ADR];				//����� ������� �� ����
		SensorFromIdx(SensCount).State = Buf[EEPROM_SENSOR_STATE];			//��������� �������
		for(u08 i=0;i<SENSOR_LEN_NAME;i++)									//��� �������
			SensorFromIdx(SensCount).Name[i] = Buf[EEPROM_SENSOR_STATE+1+i];
	}
	return EEP_READY;
}

/************************************************************************/
/* ������ �������� ����� �� EEPROM.										*/
/* ���������� 1 ���� ������� ���������, 0 - ���� �� �������             */
/************************************************************************/
u08 EeprmReadTune(void){
	u08 Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ADD_BYTE];
	
	if (EepromReady() !=0){
		for(u08 Count=0;Count<(EEPROM_TUNE_COUNT+EEPROM_TUNE_ADD_BYTE);Count++)	//������ ����� ���������
			Buf[Count] = EeprmRead(epTuneOffset+Count);
		FontIdSet(Buf[EEPROM_TUNE_FONT]);									//����� ����
		if (FontIdGet() > FONT_COUNT_MAX)									//����� �� ��� ����� ������ �����������
			FontIdSet(0);
		//u08 i = FontIdGet();
		//espUartTx(ClkIsWrite(CLK_FONT), &i, 1);
		EachHourSettingSetOff();											//�������
		if BitIsSet(Buf[EEPROM_TUNE_FLAG], EEPROM_TUNE_EACH_HOUR)
			EachHourSettingSwitch();										//�������� ���� �����
		KeyBeepSettingSetOff();												//���� ������
		if BitIsSet(Buf[EEPROM_TUNE_FLAG], EEPROM_TUNE_KEY_BEEP)
			KeyBeepSettingSwitch();											//�������� ���� �����
		HorizontalSpeed = ((u16)Buf[EEPROM_TUNE_HZ_SPEED1]<<8)+((u16)Buf[EEPROM_TUNE_HZ_SPEED2]);//�������� �������������� ���������
		if ((HorizontalSpeed >HORZ_SCROLL_MIN) || (HorizontalSpeed<HORZ_SCROLL_MAX)){	//����������� �������� �����������
			HorizontalSpeed = HORZ_SCROLL_MAX+HORZ_SCROOL_STEP*HORZ_SCROLL_STEPS/2;	//�� ��������� ������� �������� ���������
		}
#ifdef VOLUME_IS_DIGIT
		VolumeClock[vtEachHour].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_EACH_VOL];//���� ������ ��������� ��������
		VolumeClock[vtButton].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_KEY_VOL];//��������� ������
		VolumeClock[vtAlarm].Volume = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ALARM_VOL];//��������� ����������
		VolumeClock[vtAlarm].LevelVol = Buf[EEPROM_TUNE_COUNT+EEPROM_TUNE_ARARM_TYPE_VOL];//����� ����������� ��������� ����������
#endif
		return EEP_READY;
	}
	return EEP_BAD;
}

/************************************************************************/
/* ������ �������� ����� ������ ��.										*/
/* Idx - ������ ���� � ������� IRcmdList                                */
/************************************************************************/
u08 EeprmReadIRCode(const u08 Idx){

	if (EepromReady() && (Idx<IR_NUM_COMMAND)){
		IRcmdList[Idx] = 0;
		for (u08 Count=0; Count < IR_NUM_BYTS; Count++){
			IRcmdList[Idx] |= ((u32)EeprmRead(epIrCodeOffset+Count+(Idx*IR_NUM_BYTS))<<(u32)MULTIx8(Count));
		}
		return EEP_READY;
	}
	return EEP_BAD;
}

//------------------------ ������ ----------------------------------

/************************************************************************/
/* ���������� ����� � eeprom ��� ���������� ������                      */
/************************************************************************/
u16 epAdrNextPacket(void){
	if ((epAdr == EEPROM_NULL) || ((epAdr+epPacketSize) > E2END))		//����� ������ ��� �� ��������� ��� ����� ������ �� ������� ������� ������ eeprom, ����� � ������ ������ eeprom
		return EEPROM_BASE;
	else
		return epAdr+epPacketSize;										//������� ����� �������� �� ������ ������
}

/************************************************************************/
/* ������� ������ �� ������������                                       */
/************************************************************************/
u08 epPacketNewWrite(u08* Value){
	
	switch (epStepWr){
		case 0:															//�������� ������������ ������ ������
			epAdrWrite = epAdrNextPacket();								//������������ � ������ ������ ������ ������
			*Value = EEPROM_SIGNATURE;
			break;
		case 1:
			if (epAdr != EEPROM_NULL){									//��� ���������� �����, �������� ��� ��� ������������
				epAdrWrite = epAdr;
				*Value = EEPROM_NO_SIGNATURE;
				break;													//��������� ���������� switch ��� ��  �������� ������� ��������������
			}
		case 2:															//�������� ������ ���������
			epAdr = epAdrNextPacket();									//����� ������ ���������� �� ������ ������
			return EP_END;
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;
}

/************************************************************************/
/* ������ �������� ���������                                            */
/************************************************************************/
#ifdef VOLUME_IS_DIGIT
u08 epVolumeWrite(u08* Value){
	switch (epStepWr){
		case EEPROM_TUNE_EACH_VOL:
			*Value = VolumeClock[vtEachHour].Volume;					//���� ������ ��������� ��������
			break;
		case EEPROM_TUNE_KEY_VOL:
			*Value = VolumeClock[vtButton].Volume;						//��������� ������
			break;
		case EEPROM_TUNE_ALARM_VOL:
			*Value = VolumeClock[vtAlarm].Volume;						//��������� ����������
			break;
		case EEPROM_TUNE_ARARM_TYPE_VOL:
			*Value = VolumeClock[vtAlarm].LevelVol;						//����� ����������� ��������� ����������
			break;
		case (EEPROM_TUNE_ARARM_TYPE_VOL+1):							//������ �������� ��������� ���������, ������������ ����� ������������
			epEndEvent = epPacketNewWrite;
			epStepWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;											//������ � eeprom ��� �� ���������
			break;
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;													//������ � eeprom ��� �� ���������
}
#endif

/************************************************************************/
/* ������ �������� �����                                                */
/* ��� ������� ������ ���� ��������� � ������� ������ �.�. ����� ���    */
/* �������� ����� ������ ���������									    */
/************************************************************************/
u08 epTuneWrite(u08* Value){
	u08 i = 0;
	
	switch (epStepWr){
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_FONT)):	//0-� ���� - ����� ������. define EEPROM_TUNE_COUNT ������ ��� ���� ��� �� ������ ��� ����� �������� case ���� ����� ��������� ������ ��������
			*Value = FontIdGet();
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_FLAG)):	//1-� ���� - ������ ���������
			if (EachHourSettingIsSet())									//�������
				SetBit(i, EEPROM_TUNE_EACH_HOUR);
			if (KeyBeepSettingIsSet())									//���� ������� ������
				SetBit(i, EEPROM_TUNE_KEY_BEEP);
				*Value = i;
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_HZ_SPEED1)):	//2-� ���� - �������� ������� ������, ������� ����
			*Value = (u08)(HorizontalSpeed>>8);
			break;
		case (EEPROM_TUNE_COUNT-(EEPROM_TUNE_COUNT-EEPROM_TUNE_HZ_SPEED2)):	//3-� ���� - �������� ������� ������, ������� ����
			*Value = (u08)(HorizontalSpeed);
			break;
		case EEPROM_TUNE_COUNT:											//��������� ��������.
			#ifdef VOLUME_IS_DIGIT
				epEndEvent = epVolumeWrite;								//��������� ������ �������� ���������
			#else
				epEndEvent = epPacketNewWrite;							//��� �������� ������������ ������ ���� ���������� ���������� ��������� ���
			#endif
			epStepWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;
			break;		
		default:
			break;
	}
	epStepWr++;
	return EP_NORMAL;													//������ � eeprom ��� �� ���������
}

/************************************************************************/
/* ������ ����� ������ ��                                               */
/************************************************************************/
u08 epIRCodeWrite(u08* Value){

	if (epCountWr == IR_NUM_BYTS){										//��� ������� ��������� ���� �����
		epStepWr++;
		epCountWr = 0;
		if (epStepWr == IR_NUM_COMMAND) {								//��� ������� ��������� ���
			epStepWr = 0;
			epEndEvent = epTuneWrite;									//��������� ������ �������� �����
			epEndEvent(Value);
			return EP_NORMAL;
		}
	}
	*Value = (u08)(IRcmdList[epStepWr]>>MULTIx8(epCountWr) & 0xff);		//���������� ���������� ����� ���������� ����
	epCountWr++;
	return EP_NORMAL;
}

/************************************************************************/
/* ������ �������� ��������												*/
/************************************************************************/
u08 epSensorsWrite(u08* Value){

	static u08 SensorNameCount;
	
	if (epStepWr == EEPROM_SENSOR_ADR)									//������ ������ �������
		*Value = SensorFromIdx(epCountWr).Adr;
	else if (epStepWr == EEPROM_SENSOR_STATE){							//������ ��������� �������
		*Value = SensorFromIdx(epCountWr).State;
		SensorNameCount = 0;
	}
	else if ((epStepWr >EEPROM_SENSOR_STATE) && ((epStepWr-(EEPROM_SENSOR_STATE+1)) < SENSOR_LEN_NAME)){//��� �������
		*Value = SensorFromIdx(epCountWr).Name[SensorNameCount];
		SensorNameCount++;
	}
	else{																//���� ������ �������
		epCountWr++;													//��������� ������
		if (epCountWr == SENSOR_MAX){									//��� ������� ��������
			epEndEvent = epIRCodeWrite;									//��������� ������ ����� ������ ��
			epStepWr = 0;
			epCountWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;
		}
		else{															//������� ��� �� ��� ��������, ���������� ������
			epStepWr = 0;												//��������� ������ ������������
			*Value = SensorFromIdx(epCountWr).Adr;
		}
	}
	epStepWr++;
	return EP_NORMAL;													//������ � eeprom ��� �� ���������
}

/************************************************************************/
/* ������ �����������                                                   */
/************************************************************************/
u08 epAlarmWrite(u08* Value){
	
	u08 Buf[epAlarmSize];
	
	if (epStepWr == epAlarmSize){										//���� ��������� �������
		epCountWr++;													//��������� ���������
		if (epCountWr == ALARM_MAX){									//��� ���������� ��������, ��������� � ���������� ����� ������
			epEndEvent = epSensorsWrite;									
			epStepWr = 0;
			epCountWr = 0;
			epEndEvent(Value);
			return EP_NORMAL;											//������ � eeprom ��� �� ���������
		}
		epStepWr = 0;
	}
	memcpy(&Buf, ElementAlarm(epCountWr), epAlarmSize);					//��������� ��������� � �����
	*Value = Buf[epStepWr];
	epStepWr++;															//��������� ����
	return EP_NORMAL;													//������ � eeprom ��� �� ���������
}

/************************************************************************/
/* ����� ������ ��������� ����� � EEPROM. � ������ ���� ������ ���		*/
/* ���� ������ ��������� ���� � ������� �����                           */
/************************************************************************/
void EeprmStartWrite(void){
	if (epWriteIsBusy()) 												//������ ��� ����
		SetTask(EeprmStartWrite);										//������� ����������
	else{																//����� ��������� ������ � EEPROM
		epWriteBusy();													//������ �������
		epAdrWrite = epAdrNextPacket();
		epStepWr = 0;
		epCountWr = 0;
		epEndEvent = epAlarmWrite;										//������� ������������ ��������� �����������
		epAdrWrite++;													//��������� ���� ���� ��� ����� ������������ ������
		epWriteStart();													//����� ������
	}
}

/************************************************************************/
/* ������ ����� �� ������ epAdrWrite                                    */
/************************************************************************/
inline void WrieByteEeprom(u08 Value){
	while(eepromWriteBusy());											//�������� ���������� ���������� �������� � EEPROM
	EEAR = epAdrWrite;
	SetBit(EECR, EERE);
	if (EEDR != Value){													//����� �� ���������, ���� ����������
		EEDR = Value;
		SetBit(EECR, EEMWE);											//��������� ������
		SetBit(EECR, EEWE);												//�������
	}
}

/************************************************************************/
/* ������ ���������� ����� ��������� ������� ��������� ����             */
/************************************************************************/
ISR(EE_RDY_vect){
	u08 Data;
	
	if (epEndEvent(&Data) != EP_NORMAL){								//��������� ��������� ���� ��� ������ � EEPROM
		epWriteStop();													//������ �������� ���������, ��������� ���������� �� EEPROM
		epWriteFree();													//���������� �������
	}
	else{																//������ �������� ��� �� ���������, ���� �������� ���������� ����
		WrieByteEeprom(Data);
		epAdrWrite++;													//��������� �����
	}
}
