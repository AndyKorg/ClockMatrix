/*
 * sensors.c
 * ver 1.3
 */ 

#include <stddef.h>
#include "sensors.h"
#include "pff.h"
#include "eeprom.h"
#include "Sound.h"
#include "esp8266hal.h"

struct sSensor Sensors[SENSOR_MAX];

/************************************************************************/
/* ���������� ��������� �� ������ id									*/
/************************************************************************/
struct sSensor *SensorNum(u08 id){
	if (id<SENSOR_MAX)
		return &Sensors[id];
	else
		return &Sensors[0];
}


/************************************************************************/
/* �������������� ��������� �������� �������� �� ����� �� SD-�����		*/
/************************************************************************/
void SensorIni(void){
	u08 i = 0;
	
	for(;i<SENSOR_MAX;i++){
		Sensors[i].Name[0] = '�';
		Sensors[i].Name[1] = 0x30 | i;								//������ �����
		Sensors[i].Name[2] = 0;
		Sensors[i].State = 0;										//���� ��������� ��� ��� ����� ��������
		Sensors[i].Adr = SENSOR_ADR_MASK-(i<<1);					//����� ������� �� ��������� �� ����� �� 0
	}
	EeprmReadSensor();

	for(i=0;i<SENSOR_MAX;i++){
		Sensors[i].SleepPeriod = 0;														//�������� �� ���������. ������ ���� �� ������� �� ������ ��������
		Sensors[i].State = SensorFlgClr(Sensors[i]);									//����� ��������� ������� ��������, �������� ������ ���������� �����
		SensorNoInBus(Sensors[i]);														//������ ���� �� ���������
	}
}

/************************************************************************/
/* ��������� �������� �������. ���������� ��� ������ ������� ������		*/
/*  ���������� ������. 							                        */
/*  � ��������� Adr ��������� �������� ���� 1,2,3						*/
/*  � ��������� Stat ��������� �������� ���� 4,5,6,7					*/
/*  ���������� 1 ���� ������ � ������ ������������ �����������			*/
/************************************************************************/
u08 SetSensor(u08 Adr, u08 Stat, u08 Value){
	u08 Ret = SENSOR_SHOW_NORMAL;
	for(u08 i=0; i<SENSOR_MAX; i++){							//����� �������� ������� � ������ ��������
		if (Sensors[i].Adr == (Adr & SENSOR_ADR_MASK)){			//�������� ��� ����� ������� �������
			Sensors[i].State = (SensorFlgClr(Sensors[i]))		//��������� ���������� �����
								| (Stat & SENSOR_STATE_MASK);	//��������� ����� ������ �������
			Sensors[i].Value = Value;
			Sensors[i].SleepPeriod = SENSOR_MAX_PERIOD;
			espSendSensor(i);									
			if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite) && (CurrentSensorShow == i)){ //������� ������ ��������� � ������ ��������, ���� ������� ��������� �� ������� � �������
				if (Refresh != NULL){
					Refresh();
					Ret = SENSOR_SHOW_TEST;
				}
			}
			break;
		}
	}
	return Ret;
}
