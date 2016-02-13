/*
 * ���������� ��������� � ��������� ���������� ���������
 */ 

#include "VolumeHAL.h"

#define VolumePortIni		do{ SetBit(VOLUME_DDR_CLK_SI, VOLUME_CLK);\
								SetBit(VOLUME_DDR_CLK_SI, VOLUME_SI);\
								SetBit(VOLUME_DDR_CS_SHDWM, VOLUME_CS);\
								SetBit(VOLUME_DDR_CS_SHDWM, VOLUME_SHDWN);\
								ClearBit(VOLUME_PRT_CLK_SI, VOLUME_CLK);}while(0)

#define clkvolLow			ClearBit(VOLUME_PRT_CLK_SI, VOLUME_CLK)
#define clkvolHight			SetBit(VOLUME_PRT_CLK_SI, VOLUME_CLK)
#define csvolLow			ClearBit(VOLUME_PRT_CS_SHDWM, VOLUME_CS)
#define csvolHight			SetBit(VOLUME_PRT_CS_SHDWM, VOLUME_CS)

//------- �������� ������ ���������� ���������� ���������
#define VOLUME_COM_BIT0		4									//��� �0 �������
#define VOLUME_COM_BIT1		5									//��� �1 �������
#define VolWriteCommand		Bit(VOLUME_COM_BIT0)				//������� ������
#define VOLUME_COM_POT0		0									//������� ��� ������������ 0
#define VOLUME_COM_POT1		1									//������� ��� ������������ 1
#define Vol0Pot				Bit(VOLUME_COM_POT0)				//����� ������������ 0
#define Vol1Pot				Bit(VOLUME_COM_POT1)				//����� ������������ 1

/************************************************************************/
/* �������� ���� � ��������� ���������                                  */
/************************************************************************/
void VolumeOutByte(u08 Value){
	u08 i;

	for (i=8; i; i--){
		clkvolLow;
		SetBitVal(VOLUME_PRT_CLK_SI, VOLUME_SI, BitIsSet(Value, i-1)?1:0);
		clkvolHight;
	}
	clkvolLow;
}

/************************************************************************/
/* ������ ������� � ����� ������ � ��������� ���������                  */
/************************************************************************/
void VolumeCommand(const u08 Com, const u08 Dat){
	clkvolLow;
	csvolLow;
	VolumeOutByte(Com);
	VolumeOutByte(Dat);
	csvolHight;
}

/************************************************************************/
/* ������������� ���������� � ��������� ������ ��������� �� �������     */
/************************************************************************/
void VolumeIntfIni(void){
	VolumePortIni;												//��������� ����� ���������� ���������
	VolumeOffHard;												//��������� ��������� �����
}

/************************************************************************/
/* ���������� ���������                                                 */	
/************************************************************************/
void VolumeSet(u08 Volume){
	VolumeCommand((VolWriteCommand | Vol0Pot), Volume);
}