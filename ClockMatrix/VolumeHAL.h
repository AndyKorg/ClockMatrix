/*
 * ���������� ��������� � ��������� ���������� ���������
 */ 


#ifndef VOLUMEHAL_H_
#define VOLUMEHAL_H_

#include <avr/io.h>
#include "avrlibtypes.h"
#include "bits_macros.h"
#include "Clock.h"

#define VolumeOffHard		ClearBit(VOLUME_PRT_CS_SHDWM, VOLUME_SHDWN)	//���������� ���������� �����
#define VolumeOnHard		SetBit(VOLUME_PRT_CS_SHDWM, VOLUME_SHDWN)	//���������� ��������� �����

void VolumeIntfIni(void);
void VolumeSet(u08 Volume);


#endif /* VOLUMEHAL_H_ */