/*
 * CalcClock.h
 * ��������� ���������� � ������� BCD � ���������� ��� � ���� ������
 * Created: 10.05.2013 14:54:51
 *  Author: 
 */ 

#ifndef CALCCLOCK_H_
#define CALCCLOCK_H_

#include <avr/pgmspace.h>
#include "avrlibtypes.h"
#include "Clock.h"
#include "Display.h"
#include "i2c.h"

//������� ����������. ��� BCD � ���� �����! �� ����������� Yea � IsLeapYear(Yea)
#define BCDtoInt(bcd)	((((bcd & 0xf0) >> 4)*10) + (bcd & 0xf))				//BCD � Int ��� �����
#define AddDecBCD(Mc)	(((Mc & 0xf0)+0x10) & 0xf0)								//���������� �������� � ������� BCD
#define AddOneBCD(bcd)	(((bcd & 0x0f) == 9)? AddDecBCD(bcd) : (bcd+1))			//��������� bcd �� 1.
#define DecDecBCD(Mc)	(((Mc & 0xf0) == 0)?Mc : (((Mc & 0xf0)-0x10) | 0x9))	//���������� �������� � ������� BCD
#define DecOneBCD(bcd)	(((bcd & 0x0f) == 0)? DecDecBCD(bcd) : (bcd-1))			//��������� bcd �� 1.
#define IsLeapYear(Yea) ((Yea % 4) == 0)										//���������� ���. ��������� 100 � 400 �� ����������� ��������� ���� �������� ������ � 21 ����
#define LastDayFeb(Ye)	(IsLeapYear((int)(BCDtoInt(Ye)+2000))? 0x29 : 0x28)		//���������� ���� � ������� ��������� ����
#define LastDayMonth(Mo, Ye) ((Mo == 0x2)? LastDayFeb(Ye) : (((Mo == 0x4) || (Mo == 0x6) || (Mo == 0x9) || (Mo == 0x11))?0x30:0x31))  //��������� ���� ������
#define WeekInternal(wc) ((wc == 1)?7:(wc-1))									//�������� ������ ��� ������ � ������ ������� what_day, (1-��, 2-��, �.�.�.) �� ������� RTC (1-��,2-�� � �.�.)
#define WeekRTC(wc)		((wc == 7)?1:(wc+1))									//�������� ��������			

#define Tens(bcd)		((bcd >> 4) & 0xf)										//����� ����� �������� � bcd � ���� �����
#define	Unit(bcd)		(bcd & 0xf)												//����� ����� ������

#define SEC_IN_MIN		60														//������ � ������
#define SEC_IN_HOUR		3600													//������ � ����
#define YEAR_LIMIT		0x13													//��� �������������� ������� ��������� ��� � �������� ������ � dateime, � ������� BCD
#define DAYS_01_01_2013	41272UL													//������ ���������� ���� ��������� � 01.01.1900 �� 01.01.2013
#define SEC_IN_DAY		86400UL													//������ � ������

u08 what_day(const u08 date, const u08 month, const u08 year);					//���� ������ �� ����. ���������� 1-��, 2-�� � �.�. 7-��
u08 AddClock(volatile struct sClockValue *Clck, 								//�������������� ��������� �������� �������� � ������������ � ����� (�������� ��� ����� ����� 59 ���� 00)
					enum tSetStatus _SetStatus);
u08 DecClock(volatile struct sClockValue *Clck,									//�������������� ���������� �������� ��������
					enum tSetStatus _SetStatus);
					
void AddNameMonth(u08 Month);													//�������� �������� ������ � ������ ������
void AddNameWeekFull(u08 NumDay);												//�������� �������� ������
#define PLACE_HOURS		0														//���������� ����� �� ���������� �����
#define PLACE_MINUT		1														//���������� ����� �� ���������� �����
void PlaceDay(const u08 NumDay, const u08 PlaceDigit);							//��������� ������������� �������� ��� ������

u32 bin2bcd_u32(u32 data, u08 result_bytes);									//��������� ��������� ����� � BCD
u08 HourToInt(const u08 Hour);													//��������� ����� ����� � �����
u08 SecundToDateTime(u32 Secunds, volatile struct sClockValue *Value, s08 HourOffset);	//��������� ����� ������ ��������� � 1.1.1900 � ���� � ����� � ������ �������� �����

#endif /* CALCCLOCK_H_ */