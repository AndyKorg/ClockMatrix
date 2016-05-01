/*
 * ������������ ������� ��������
 * �����������, �������� � �.�.
 * ��������� ����������� ������� ���������������� ��������� �������:
 * - ��� ������� ������������ ��� "�N" ��� N ���������� ������ ��������� � ������� ��������, �� 0 �� SENSOR_MAX
 * - ����� ������� �� ���� �� SENSOR_ADR_MASK �� 0 � ����� 2. �������� �������� ��� ����� ������� �� 1 ��� ����� � ������� ��� ������ 0
 */ 


#ifndef SENSORS_H_
#define SENSORS_H_

#include "Clock.h"

// ������ ��������: 
//	������������ ����� ������� ������������ ������ 1,2,3 ���������� �� ���� ��� ������. 
//	�������� ��� SENSOR_DS18B20 ��� ����� 6 => 0b00001100 ����� ����� 1,2,3(d0b00001110) => ����� �� ���� ��� ����� => 0b00000110=6
//	�������������� ������ ���������� ������� �� �����������. ��� ��������� ������������ ��������� �� �� ������� ������������.
#define SENSOR_DS18B20		0b00001100				// �� ���� 1-ware ������ ���� ����� ���� ������ ����, � ������� 6.
#define SENSOR_BMP180		0b00000110				//����� BMP180 ��� �����������, �������� � ��������� sSensor, �� ������ � BMP180_ADDRESS!
#define BMP180_ADDRESS		0b11101110				//����� BMP180 �� ���� I2C, ����� � ����� �������
#define SENSOR_LM75AD		0b10011110				//����� LM75AD

#define SENSOR_MAX			3						//���������� �������������� ��������, �� ����� 8. ��� ����������� ���������, �� �� ����� ���� ��� � ����� ��������� ������ � ���������� ������ ������
#if (SENSOR_MAX>8)
	#error "Value of the SENSOR_MAX must be less than 8"	//�������� ���� ���������� �������� ������� �� ����������� ����������� ������
#endif

#define SENSOR_LEN_NAME		3						//����� ����� �������
#define SENSOR_MAX_PERIOD	30						//������ � ������� � ������� �������� �� ��������� ���������� �� �������. �� ��������� ����� �������, ���������� ��������� ����������������
#define SENSOR_TEST_REPEAT	500						//������ ���������� ��������� ��� ������ ������������

struct sSensor{										//������ �����������, �������� � �.�. � eeprom ����������� ������ Adr, State, Name
	u08 Adr;										//����� �������. ��������� �������� ���� 1,2,3. ��� 0 ������ ����� 0 � ����������� ����� ������� �� 1 �����. ��� ������� ��� ���� ��� �� ��������� ������ �� ������� � �� ��-���������
	u08 Value;										//�������� ���������� ��������, ��� ������� �������� �������� �� PressureBase � ��.��.��
	u08 State;										//��������� �������. ������������, ������� ��������� � ��. ������ ������� 4 ���� ������������ ������� ��������, � ������� 4 ����������� ������ ��������� �����
	u08 SleepPeriod;								//������� ��������� ������� ������� � ������� �������� �� �������� ������ �� �������. � �������
	char Name[SENSOR_LEN_NAME];						//��� ������� ��� ������ �� �������
};

#define PressureBase	641							//�������� ��� �������� �������� ��������, ��� �� ��������� ����������� � 32 ��� �� 16
#define PressureNormal(shortPress)	((u16)(PressureBase+shortPress))
#define PressureShort(pressure)	((u08)(pressure-PressureBase))

//------------------------------- ���� ����� State � ��������� sSensor
// TODO: ���������� �������� ��������� ����� ������� � ���� ���������� ���� � ��������� �������
													//���� �� 7 �� 4 ��������� �� �������
#define	SENSOR_NO_SENSOR	7						//������ �� ��������� 1 - ��� ������� �� ���� (� ������������ ��� ��������, ��� ����� �� ������������ ����, �� ������ ���� ������ �� �������)
#define	SENSOR_LOW_POWER	6						//������ ���������� ��������� 1 - ������� ������ ���������� �� �������
#define	SENSOR_NO_RF		5						//����� ����� ������ ��� ������������. 1 - ����������� �� ������� ������� ����� ������
#define	SENSOR_PRESSURE		4						//��� �������: 1-������ ��������
													//���� �� 3 �� 0 �������������� ������ ������
#define SENSOR_SHOW_ON		3						//�������� ����� �������� ������� �� �������
#define SENSOR_RESERV_CLK1	2						//-------- ��������������� ��� �������� �������������
#define SENSOR_RESERV_CLK2	1
#define SENSOR_RESERV_CLK3	0

#define SENSOR_ADR_MASK		0b00001110				//����� ������ �������
#define SENSOR_STATE_MASK	((1<<SENSOR_NO_SENSOR) | (1<<SENSOR_LOW_POWER) | (1<<SENSOR_NO_RF) | (1<<SENSOR_PRESSURE))	//����� ������ ������� �������� (�� ���������� �����!)
#define SensorSetInBus(st)	ClearBit(st.State, SENSOR_NO_SENSOR)		//������ �� ���� ����. ��� ������������ ��� �������� ��� ���� ����� �� ����������������, �� ����� ��� ������ �� ������ �������. ������ ������� � ������������ ����������� ����� �������� Value (��. ����� TMPR_NO_SENS...)
#define SensorNoInBus(st)	SetBit(st.State, SENSOR_NO_SENSOR)			//�������� ���������� ������� �� ����
#define SensorFlgClr(st)	(st.State & (~SENSOR_STATE_MASK))			//����� ������ ������ �������
#define SensorIsNo(st)		BitIsSet(st.State, SENSOR_NO_SENSOR)		//������ ����������� �� ����?
#define SensorIsSet(st)		BitIsClear(st.State, SENSOR_NO_SENSOR)		//������ �������� �� ����?
#define	SensorBatareyIsLow(st)	BitIsSet(st.State, SENSOR_LOW_POWER)	//������� ���������
#define SensotTypeTemp(st)	ClearBit(st.State, SENSOR_PRESSURE)			//��� ������ �����������
#define SensorTypePress(st)	SetBit(st.State, SENSOR_PRESSURE)			//��� ������ ��������
#define SensorIsPress(st)	BitIsSet(st.State, SENSOR_PRESSURE)			//��� ������ ��������?
#define SensorRFInBus(st)	ClearBit(st.State, SENSOR_NO_RF)			//����������� ������� �� �������
#define SensorRFNoBus(st)	SetBit(st.State, SENSOR_NO_RF)
#define SensorRFIsNo(st)	BitIsSet(st.State, SENSOR_NO_RF)

#define SensorShowOn(st)		SetBit(st.State, SENSOR_SHOW_ON)		//�������� ����������� ������� �� �������
#define SensorShowOff(st)		ClearBit(st.State, SENSOR_SHOW_ON)		//���������
#define SensorIsShow(st)		BitIsSet(st.State, SENSOR_SHOW_ON)		//����� �� ������� ��� ������� �������

//������� ��� ������� ����������� �������� ��������. �������� ������� ������ �� ���� ��� ������������� ����������� ��������� � �������������� ���� �� 0xc90 (-55 C) �� 0xff (-1 C)
#define TMPR_NO_SENS		0x80					//��� �������

//������ ����������� �������
#define SENSOR_SHOW_NORMAL	0						//����������� �� ���� ������������� (� ������� ������ ��������)
#define SENSOR_SHOW_TEST	1						//����� ������������ ����������� ���������� ���������

void SensorIni(void);								//������������� �������� ��������
u08 SetSensor(u08 Adr, u08 Stat, u08 Value);		//������ ������-������� � �������� �������, ����������� �������� ������ ����������� �������� �������
struct sSensor *SensorNum(u08 id);					//���������� ��������� �� ������ id

#define SensorFromIdx(id)	(*SensorNum(id))

#endif /* SENSORS_H_ */