/*
 * i2c_ExtTmpr.c
 * ������� �������� ����������� �� LM75AD
 * ��������� ����������� ������� ����������. ������� ��������� ����������� �� ��� ��� ���� �� ����� ������� ���������.
 * v.1.3
 */ 

#include "avrlibtypes.h"
#include "bits_macros.h"
#include "i2c.h"
#include "EERTOS.h"
#include "sensors.h"

/************************************************************************/
/* �������� ������ ���������                                            */
/************************************************************************/
void i2c_OK_ExtTmprFunc(void){
	struct sSensor Sens;											//���� ���� ���������, ��� �� ���� �����, ��� ����� �������� �� �������� �������
	Sens.State = 0;
	SensorSetInBus(Sens);											//������ �� ���� ���������
	SensotTypeTemp(Sens);											//������ �����������
	if (SetSensor(SENSOR_LM75AD, Sens.State, i2c_Buffer[0]) == SENSOR_SHOW_TEST){
		SetTimerTask(i2c_ExtTmpr_Read, SENSOR_TEST_REPEAT);			//� ������ ������������ ��������� ����� ��������� ����������
	}
	i2c_Do &= i2c_Free;												//����������� ����
}

/************************************************************************/
/* ������ ������			                                            */
/************************************************************************/
void i2c_Err_ExtTmprFunc(void){
	static u08 Attempt = 3;
	
	if (i2c_Do & i2c_ERR_NA){										//���������� �� �������� ��� �����������, ��������� ��� ��� ����
		Attempt--;
		if (Attempt)
			SetTimerTask(i2c_ExtTmpr_Read, 2*REFRESH_CLOCK);		//������� ��������� ��������
		else{														//����� ���� ��������� ������� ����� ������� ������������, �� ��� ������������ �����������
			struct sSensor Sens;									//���� ���� ���������, ��� �� ���� �����, ��� ����� �������� �� �������� �������
			Sens.State = 0;
			SensorNoInBus(Sens);									//������ �� ���� �� ���������
			SensotTypeTemp(Sens);									//������ �����������
			if (SetSensor(SENSOR_LM75AD, Sens.State, 0) == SENSOR_SHOW_TEST){	//��������� ��������
				SetTask(i2c_ExtTmpr_Read);							//� ������ ������������ ��������� ����� ��������� ����������
				Attempt = 0;
			}
		}
	}																
	else	
		SetTimerTask(i2c_ExtTmpr_Read, REFRESH_CLOCK);				//������� ��������� ��������
	i2c_Do &= i2c_Free;												//����������� ����
}

/************************************************************************/
/* ������ �������� �� ������� ����������� �� ���� i2c				    */
/************************************************************************/
void i2c_ExtTmpr_Read(void){
	if (i2c_Do & i2c_Busy){											//���� ������, ���������� �����
		SetTimerTask(i2c_ExtTmpr_Read, REFRESH_CLOCK);
		return;
	}
	i2c_SlaveAddress = SENSOR_LM75AD;
	i2c_index = 0;													//������ � ����� � ����
	i2c_Do &= ~i2c_type_msk;										//����� ������
	i2c_ByteCount = i2c_ExtTmprBuffer;								//���������� �������� ����
	i2c_PageAddrCount = 1;											//����� ������ ��������
	i2c_PageAddrIndex = 0;											//��������� ������ ������ � ������ ������
	i2c_PageAddress[0] = extmpradrTEMP;								//����� �������� �� �������� ����� ������
	i2c_Do |= i2c_sawsarp;											//����� ������ ������.

	MasterOutFunc = &i2c_OK_ExtTmprFunc;
	ErrorOutFunc = &i2c_Err_ExtTmprFunc;
	
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //���� ��������
	i2c_Do |= i2c_Busy;
}
