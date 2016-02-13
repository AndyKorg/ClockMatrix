/*
 * ����� ������ ������� ������ ������ ������ �� ���������.
 * ������������ ������� ���������� �� ���������� 32 768 �� ��� 
 * ���������� �������, �.�. ����������� �� �������� ���������� ����� �� ������� ��� ��������������� �����. 
 * ������ �� ���������� ����� ��
 * ���� �������� ���������� INT1
 * ver.1.3	
 */ 


#ifndef DISPLAYHAL_H_
#define DISPLAYHAL_H_

#include "Clock.h"
#include "IRrecive.h"							//���������� ��������� ������������ � ������� ���������

#if (defined(SHIFT_REG_TYPE_HC595) && defined(SHIFT_REG_TYPE_MBI50XX))
#error "one shall be defined only or SHIFT_REG_TYPE_HC595 or SHIFT_REG_TYPE_MBI50XX"
#endif

#if (defined(LED_MATRIX_COMMON_ROW) && defined(LED_MATRIX_COMMON_COLUMN))
#error "one shall be defined only or LED_MATRIX_COMMON_ROW or LED_MATRIX_COMMON_COLUMN"
#endif

extern u08 DisplayBuf[DISP_COL];				//����� ������������� ������

void DispRefreshIni(void);						//������������� ����������� �������
void ClearScreen(void);							//������� ������ ������

#endif /* DISPLAYHAL_H_ */