/*
 * ��������� ����������� ��������� �� ������������ ds18b20
 * ���� ������ �� ���� ����� ���� ������ ���� � ��� ����� 0x0c
 * 
 * ver. 1.0
 */ 


#ifndef DS18B20_H_
#define DS18B20_H_

#define SENSOR_DS18B20		0b00001100				//����� (0x6, ������� ����� � ��������� ��� �������) ������������ �� ���� 1-ware, ���� ������ ������. //TODO:���������� �� ������������ ����������� �������� �� ���� 1-ware

void StartMeasureDS18(void);					//��������� ��������� 

#endif /* DS18B20_H_ */