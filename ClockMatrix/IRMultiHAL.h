/*
 * ������������� ������� ������� ��-�������
 * ������� �� ������ http://we.easyelectronics.ru/Soft/prostoy-universalnyy-dekoder-ik-du.html
 * ��������������, ��� ������������ ��������� ��� 0 � 1 ����������� �������������
 * 1.152 �� � ��� ����������� ����� ����� ����������� ������ �� ���������. ���� ����� ����� ����������� ������ ����� ������, �� ��������� � ��� 1, ���� ������ � �� 0.
 * ����� ���� ������ 19.2 ��, �.�. � ���������� ��������� ������� ���� � ������ ������ ������� ��������/�����.
 * ��� �� �� �� ������� �� ����� ������ ������������ �������� ������ ���� �������� � 2 ���� ������.
 * �������������� ��� ����� �������� �������� 4 �����, ������� ���������� code � rx_buffer 32-� ���������
 * ��� ������� ���������� ������������ ���������� �������� 32 ��� �� ���������� RTC (������ 1/32768 �� = 30,517578125 ���)
 *
 * � ���� �� ������ ���������� ���������� �������������� ����������� ������ � ������� �� ��� �������� � ������ ��������� ������ �� ������
 * �� ���������� �������� ������ ������ �� ���������� �������, � � ����� � �������� �� ��������, �.�. �������� ���������� ������� ��������� � ���� ������
 *  �� ��������� �������� ���������� ���� ������������ ������, � ������ �� ���-�������.
 */ 

#ifndef IRMULTIHAL_H_
#define IRMULTIHAL_H_

#include <avr/io.h>
#include <stdlib.h>
#include "EERTOS.h"
#include "IRrecive.h"

void IRRecivHALInit(void);

#endif //IRMULTIHAL_H_