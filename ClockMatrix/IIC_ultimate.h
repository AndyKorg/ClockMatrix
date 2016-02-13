/*
		����� DI HALT (http://easyelectronics.ru)
		����: C
		������������ 23 ������ 2010 ���� � 22:56
		����� ��� ���������� IIC
*/

#ifndef IICULTIMATE_H
#define IICULTIMATE_H
  
#include <avr/io.h>
#include <avr/interrupt.h>
#include "avrlibdefs.h"
#include "avrlibtypes.h"
 
//(Platform dependent)
#define i2c_PORT	PORTC		// ���� ��� ����� ���� TWI
#define i2c_DDR		DDRC
#define i2c_SCL		PORTC5		// ���� ��������������� �������
#define i2c_SDA		PORTC4
 
 
#define i2c_MasterAddress 	1 //0x32	// ����� �� ������� ����� ����������
#define i2c_i_am_slave		0			// ���� �� ��� � ������� �������� �� 1. � �� �� �������!
 
#define i2c_MasterBytesRX	0x0			// �������� ������������ ������ ������ Slave, �.�. ������� ���� ����.
#define i2c_MasterBytesTX	0x0			// �������� ����������� ������ ������ Slave , �.�. ������� ���� ������ �� ������.
 
#define i2c_MaxBuffer		0x12		// �������� ������ Master ������ RX-TX. ������� �� ���� ����� ����� ������ �� ����� ������
#define i2c_MaxPageAddrLgth	1			// ������������ �������� ������ ��������. ������ ����� �������� ��� ���� ��� ��� �����. 
										// ������� �� ���� ������ ��� ������ ����������. 
  
#define i2c_type_msk	0b00001100	// ����� ������
#define i2c_sarp	0b00000000	// Start-Addr_R-Read-Stop  							��� ����� �������� ������. �������� �� ������ ��� �� ������ � �������� ������
#define i2c_sawp	0b00000100	// Start-Addr_W-Write-Stop 							��� ����� ������� ������. � ��� ����� � ������ � ������� ��������. 
#define i2c_sawsarp	0b00001000	// Start-Addr_W-WrPageAdr-rStart-Addr_R-Read-Stop 	��� ����� � ��������������� ������� �������� ������ ��������
 
#define i2c_Err_msk	0b00110011	// ����� ���� ������
#define i2c_Err_NO	0b00000000	// All Right! 						-- ��� ����, �������� �������. 
#define i2c_ERR_NA	0b00010000	// Device No Answer 				-- ����� �� ��������. �.�. ���� �����, ���� ��� ��� �� �����.
#define i2c_ERR_LP	0b00100000	// Low Priority 					-- ��� ����������� ����������� �������, ���� �� ��������� ��������
#define i2c_ERR_NK	0b00000010	// Received NACK. End Transmittion. -- ��� ������� NACK. ������ � ���.
#define i2c_ERR_BF	0b00000001	// BUS FAIL 						-- ������� ��������. � ���� ��� �������. ����� ����������� ������� ����������������� ����
 
#define i2c_Interrupted		0b10000000		// Transmiting Interrupted		�������� ��������� ����� ���������
#define i2c_NoInterrupted 	0b01111111  	// Transmiting No Interrupted	�������� ������ ����� ���������
 
#define i2c_Busy		0b01000000  	// Trans is Busy				�������� ����� "���������� �����, ������ �� �������". 
#define i2c_Free		0b10111111  	// Trans is Free				�������� ������ ����� ���������.
 
 
#define MACRO_i2c_WhatDo_MasterOut 	(MasterOutFunc)();		// ������� ��� ������ ������. ���� ��� �������, �� ����� ���� ��� ������
#define MACRO_i2c_WhatDo_SlaveOut   	(SlaveOutFunc)();
#define MACRO_i2c_WhatDo_ErrorOut   	(ErrorOutFunc)();
 
 
 
typedef void (*IIC_F)(void);		// ��� ��������� �� �������
 
extern IIC_F MasterOutFunc;			// ����������� � �������. 
extern IIC_F SlaveOutFunc;
extern IIC_F ErrorOutFunc;
 
 
extern volatile u08 i2c_Do;											
extern u08 i2c_InBuff[i2c_MasterBytesRX];
extern u08 i2c_OutBuff[i2c_MasterBytesTX];
 
extern u08 i2c_SlaveIndex;
 
 
extern u08 i2c_Buffer[i2c_MaxBuffer];
extern u08 i2c_index;
extern u08 i2c_ByteCount;
 
extern u08 i2c_SlaveAddress;
extern u08 i2c_PageAddress[i2c_MaxPageAddrLgth];
 
extern u08 i2c_PageAddrIndex;
extern u08 i2c_PageAddrCount;
 
 
extern void Init_i2c(void);
extern void Init_Slave_i2c(IIC_F Addr);

#ifdef DEBUG
//#define DEBUG_I2C
#endif
 
#ifdef DEBUG_I2C
extern u08 	WorkLog[100];
extern u08	WorkIndex;
#endif 
 
#endif