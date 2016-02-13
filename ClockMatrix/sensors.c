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

// ������ ������ �������� ������� �� sd ����� � ����� sensor.txt: 1=1��, ��� 1 - ����� �������, = ���� �����, �� ������� ���� �� ����� ���� ���� �������� �������
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
	//TODO: �������� ����� ���� ������� ��� ��������� SD ���/����� � �����
//	FATFS fs;
//	WORD rb;														//����������� ����
//	u08 CountSens = 0;
	u08 i = 0;
	
	for(;i<SENSOR_MAX;i++){
		Sensors[i].Name[0] = '�';
		Sensors[i].Name[1] = 0x30 | i;								//������ �����
		Sensors[i].Name[2] = 0;
		Sensors[i].State = 0;										//���� ��������� ��� ��� ����� ��������
		Sensors[i].Adr = SENSOR_ADR_MASK-(i<<1);					//����� ������� �� ��������� �� ����� �� 0
	}
	EeprmReadSensor();
/*	
	if (!EeprmReadSensor()){										//�� ������� ��������� ��������� �������� �� eeprom, ���� �� sd-�����
		if (pf_disk_is_mount() != FR_OK)							//����� ��� �� ������������, ������� �� ����������������.
			if (pf_mount(&fs) == FR_OK)
				if (pf_open("sensor.txt") == FR_OK)
					if (pf_lseek (0) == FR_OK)
						if (pf_read(&read_buf0, BUF_LEN, &rb) == FR_OK){
							while(i <= rb){
								if	( (read_buf0[i] >= 0x30) && (read_buf0[i]<=0x39)	//������ ������ �����, 
									  && (read_buf0[i+1] == '=')						//� ������ ���� �����
									){													//������ ������
									Sensors[CountSens].Adr = ((read_buf0[i] & 0xf)<<1);	//����� ������� � ������ ��������
									for(u08 j = 0;j < SENSOR_LEN_NAME; j++)				//����������� ������ ��� ����� �������
										Sensors[CountSens].Name[j] = 0;
									for(u08 j = 0;j < SENSOR_LEN_NAME; j++){			//��������� ��� �������
										if ((read_buf0[(i+2)+j] == 0xd) || (read_buf0[(i+2)+j] == 0xa) || (read_buf0[(i+2)+j] == 0) || ((i+2+j)>rb) )
											break;										//����� ������ ��� �����
										Sensors[CountSens].Name[j] = read_buf0[(i+2)+j];
									}
									SensorShowOn(Sensors[CountSens]);					//�������� ����� �������
									CountSens++;										//��������� �������� �������
									if (CountSens > SENSOR_MAX) break;					//������ ��� �������� ��������
								}
								while((read_buf0[i] != 0xd) && (i<=rb)) i++;			//��������� ������
								if (read_buf0[i] == 0xa) i++;							//���������� ��������� ������ ���� �� ����
								if (i>=rb) break;										//�������� ����� ������
							}
						}
	}
	*/
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
/*  � ��������� Stat ��������� �������� ���� 4,5,6,7					 */
/************************************************************************/
void SetSensor(u08 Adr, u08 Stat, u08 Value){
	for(u08 i=0; i<SENSOR_MAX; i++){							//����� �������� ������� � ������ ��������
		if (Sensors[i].Adr == (Adr & SENSOR_ADR_MASK)){			//�������� ��� ����� ������� �������
			Sensors[i].State = (SensorFlgClr(Sensors[i]))		//��������� ���������� �����
								| (Stat & SENSOR_STATE_MASK);	//��������� ����� ������ �������
			Sensors[i].Value = Value;
			Sensors[i].SleepPeriod = SENSOR_MAX_PERIOD;
			espSendSensor(i);
			if (SensorTestModeIsOn(Sensors[i])){				//������� ������ ��������� � ������ ����, ���� ������� ��������� �� ������� � �������
				SetClockStatus(csTune, ssSensorTest);
				SoundOn(SND_SENSOR_TEST);
			}
			break;
		}
	}
}
