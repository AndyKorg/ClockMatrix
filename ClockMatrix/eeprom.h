/*
 * �������� �������� ����� � EEPROM
 * ������������ ���� ������ eeprom ��� ������������ ������������ ������� flash-�����. �� ���� �� ������� ��� ������ �����, �� ���������.
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include "Clock.h"
#include "Alarm.h"

#define EEP_READY		1							//������ ���������
#define EEP_BAD			0							//� eeprom ��� ��������� ������

void EepromInit(void);								//������������� ���������� � ��������� EEPROM
u08 EeprmReadAlarm(struct sAlarm* Alrm);			//������ �������� ����������� �� EEPROM
u08 EeprmReadSensor(void);							//������ �������� ��������
u08 EeprmReadIRCode(u08 Idx);						//������ ����� ������ ��
u08 EeprmReadTune(void);							//������ �������� �����
void EeprmStartWrite(void);							//������ ������ ��������� � EEPROM. ������������ ��� ����� - ���������� � ��������� �����

#endif /* EEPROM_H_ */