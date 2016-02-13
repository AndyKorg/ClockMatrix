/***********************************************************
	����� RTOS
	����� DI-HALT http://easyelectronics.ru/
***********************************************************/
#include "EERTOSHAL.h"

//RTOS ������ ���������� �������
inline void RunRTOS (void)
{
/* ������ RTOS ������ �� ������� ������� �������, �.�. ������������ ���������� �� 32 ���
SetTimerCTCMode;								// Freq = CK/64 - ���������� ����� � ���� ��������
SetTimerPresclr;
												// ���� ����� ����� ���������� �������� ���������
SERVICE_TIMER_TCTNT = 0;						// ���������� ��������� �������� ���������
SERVICE_TIMER_OCR  = TimerDivider;				// ���������� �������� � ������� ���������
SERVICE_TIMER_TMISK |= (1<<SERVICE_TIMER_OCIE);	// ��������� ���������� �� ��������� 
*/
Enable_Interrupt;								//��������� ���������� RTOS - ������ ��
}
