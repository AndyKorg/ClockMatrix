/*
 * �������� � Timer2Ctrl.h
 * ver. 1.0
 */ 

#include <avr/interrupt.h>
#include "Timer2Ctrl.h"

volatile TM_OWR_EVNT			EndDelayEvent;					//������ �� ������� ���������� � ����� ��������.

volatile u08 TimerFlag;

ISR(OW_DELAY_INT){
	EndDelayEvent(FUNC_DELAY_NORMAL);
}
