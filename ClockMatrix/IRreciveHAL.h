/*
 * IRrecive.h
 * ver 1.2
 */ 


#ifndef IRRECIVEHAL_H_
#define IRRECIVEHAL_H_

#include <avr/interrupt.h>
#include <stddef.h>
#include "bits_macros.h"
#include "avrlibtypes.h"
#include "DisplayHAL.h"				//������ �������� ��������� � ������� ���������� �� ��������� ���������� ���������� RTC - 32 768 �� (���������� PeriodIR)
#include "IRrecive.h"
#include "Clock.h"

void IRRecivHALInit(void);			//������������� ��������� ��

u08 ConvertIRCodeToDec(u08 Code);	//�������������� ���� �������� ������� � �����, ���������� 10 ���� ��� �� ������������� �����

#endif /* IRRECIVEHAL_H_ */