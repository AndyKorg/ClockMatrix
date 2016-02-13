/*
		����� DI HALT (http://easyelectronics.ru)
		����: C
		������������ 23 ������ 2010 ���� � 22:20
		�������� �������, � ����� ��������� ������������� TWI ���������� Mega AVR
*/

#include "IIC_ultimate.h"

void DoNothing(void);
 
volatile u08 i2c_Do;						// ���������� ��������� ����������� IIC
u08 i2c_InBuff[i2c_MasterBytesRX];			// ����� ����� ��� ������ ��� Slave
u08 i2c_OutBuff[i2c_MasterBytesTX];			// ����� �������� ��� ������ ��� Slave
u08 i2c_SlaveIndex;							// ������ ������ Slave
 
 
u08 i2c_Buffer[i2c_MaxBuffer];				// ����� ��� ������ ������ � ������ Master
u08 i2c_index;								// ������ ����� ������
u08 i2c_ByteCount;							// ����� ���� ������������
 
u08 i2c_SlaveAddress;						// ����� ������������
 
u08 i2c_PageAddress[i2c_MaxPageAddrLgth];	// ����� ������ ������� (��� ������ � sawsarp)
u08 i2c_PageAddrIndex;						// ������ ������ ������ �������
u08 i2c_PageAddrCount;						// ����� ���� � ������ �������� ��� �������� Slave
 
											// ��������� ������ �� ��������:
IIC_F MasterOutFunc = &DoNothing;			//  � Master ������
IIC_F SlaveOutFunc 	= &DoNothing;			//  � ������ Slave
IIC_F ErrorOutFunc 	= &DoNothing;			//  � ���������� ������ � ������ Master
 
 
#ifdef DEBUG_I2C
u08 	WorkLog[100];						// ��� ����� ����
u08		WorkIndex=0;						// ������ ����
#endif 
 
ISR(TWI_vect)								// ���������� TWI ��� ���� ���.
{

/*
PORTB ^= 0x01;					// ������� ����� �����, ��� ������������� ����������� ����������� � ������� ������ TWI
*/ 
 
#ifdef DEBUG_I2C
// ���������� �����. ����� ���� ������ ��������� �������� � ����� ������, � �����. �� ��������� ������ ����� UART �� ����
if (WorkIndex <99)				// ���� ��� �� ����������
{
	WorkLog[WorkIndex]= TWSR;	// ����� ������ � ���
	WorkIndex++;
}

#endif

switch(TWSR & 0xF8)											// �������� ���� ����������
	{
	case 0x00:	// Bus Fail (������� ��������)
			{
			i2c_Do |= i2c_ERR_BF;
 
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;  	// Go!
 
			MACRO_i2c_WhatDo_ErrorOut
			break;
			}
 
	case 0x08:	// ����� ���, � ����� ��:
			{
			if( (i2c_Do & i2c_type_msk)== i2c_sarp)			// � ����������� �� ������
				{
				i2c_SlaveAddress |= 0x01;					// ���� Addr+R
				}
			else											// ��� 
				{
				i2c_SlaveAddress &= 0xFE;					// ���� Addr+W
				}
 
			TWDR = i2c_SlaveAddress;				// ����� ������
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;  								// Go!
			break;
			}
 
	case 0x10:	// ��������� ����� ���, � ����� ��
			{
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)		// � ����������� �� ������
				{
				i2c_SlaveAddress |= 0x01;					// ���� Addr+R
				}
			else
				{
				i2c_SlaveAddress &= 0xFE;					// ���� Addr+W
				}
 
			// To Do: �������� ���� ��������� ������ 
 
			TWDR = i2c_SlaveAddress;				// ����� ������
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;  	// Go!
			break;
			}
 
	case 0x18:	// ��� ������ SLA+W �������� ACK, � �����:
			{
			if( (i2c_Do & i2c_type_msk) == i2c_sawp)		// � ����������� �� ������
				{
				TWDR = i2c_Buffer[i2c_index];				// ���� ���� ������
				i2c_index++;							// ����������� ��������� ������
 
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					i2c_i_am_slave<<TWEA|
					1<<TWEN|
					1<<TWIE;  // Go! 
 
				}
 
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)
				{
				TWDR = i2c_PageAddress[i2c_PageAddrIndex];	// ��� ���� ����� ������� (�� ���� ���� ���� ������)
				i2c_PageAddrIndex++;
															// ����������� ��������� ������ ��������
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					i2c_i_am_slave<<TWEA|
					1<<TWEN|
					1<<TWIE;	// Go!
				}
			}
			break;
 
	case 0x20:	// ��� ������ SLA+W �������� NACK - ����� ���� �����, ���� ��� ��� ����.
			{
			i2c_Do |= i2c_ERR_NA;	
															// ��� ������
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;								// ���� ���� Stop
 
			MACRO_i2c_WhatDo_ErrorOut 						// ������������ ������� ������;
			break;
			}
 
	case 0x28: 	// ���� ������ �������, �������� ACK!  (���� sawp - ��� ��� ���� ������. ���� sawsarp - ���� ������ ��������)
			{	// � ������: 
			if( (i2c_Do & i2c_type_msk) == i2c_sawp)		// � ����������� �� ������
				{
				if (i2c_index == i2c_ByteCount)				// ���� ��� ���� ������ ���������
					{																		
					TWCR = 	0<<TWSTA|
						1<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;						// ���� Stop
 
					MACRO_i2c_WhatDo_MasterOut				// � ������� � ��������� �����
 
					}
				else
					{
					TWDR = i2c_Buffer[i2c_index];			// ���� ���� ��� ���� ����
					i2c_index++;
					TWCR = 	0<<TWSTA|						
						0<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;  						// Go!
					}
				}
 
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)		// � ������ ������ ��
				{
				if(i2c_PageAddrIndex == i2c_PageAddrCount)	// ���� ��������� ���� ������ ��������
					{
					TWCR = 	1<<TWSTA|
						0<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;						// ��������� ��������� �����!
					}
				else
					{												// ����� 
					TWDR = i2c_PageAddress[i2c_PageAddrIndex];		// ���� ��� ���� ����� ��������
					i2c_PageAddrIndex++;							// ����������� ������ �������� ������ �������
					TWCR = 	0<<TWSTA|
						0<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;						// Go!
					}
				}	 
			}
			break;
 
	case 0x30:	//���� ����, �� �������� NACK ������ ���. 1� �������� �������� ������� � ��� ����. 2� ����� �������.
			{
			i2c_Do |= i2c_ERR_NK;						// ������� ������ ������. ���� ��� �� ����, ��� ������. 
 
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// ���� Stop
 
			MACRO_i2c_WhatDo_MasterOut					// ������������ ������� ������
 
			break;
			}
 
	case 0x38:	//  �������� �� ����. ������� ��� �� ���������
			{
			i2c_Do |= i2c_ERR_LP;						// ������ ������ ������ ����������
 
			// ����������� ������� ������. 
			i2c_index = 0;
			i2c_PageAddrIndex = 0;
 
			TWCR = 	1<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// ��� ������ ���� ����� �������� 
			break;										// ��������� �������� �����.
			}
 
	case 0x40: // ������� SLA+R �������� ���. � ������ ����� �������� �����
			{
			if(i2c_index+1 == i2c_ByteCount)			// ���� ����� �������� �� ���� �����, �� 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ������� ����, � � ����� ����� ������ NACK(Disconnect)
				}										// ��� ���� ������ ������, ��� ��� ������ �����. � �� �������� ����
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ��� ������ ������ ���� � ������ ����� ACK
				}
 
			break;
			}
 
	case 0x48: // ������� SLA+R, �� �������� NACK. ������ slave ����� ��� ��� ��� ����. 
			{
			i2c_Do |= i2c_ERR_NA;
														// ��� ������ No Answer
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// ���� Stop
 
			MACRO_i2c_WhatDo_ErrorOut					// ������������ �������� �������� ������
			break;
			}
 
	case 0x50: // ������� ����.
			{ 
			i2c_Buffer[i2c_index] = TWDR;				// ������� ��� �� ������
			i2c_index++;
 
			// To Do: �������� �������� ������������ ������. � �� ���� �� ��� ���� ���������
 
			if (i2c_index+1 == i2c_ByteCount)			// ���� ������� ��� ���� ���� �� ���, ��� �� ������ �������
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ����������� ��� � ����� ������ NACK (Disconnect)
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ���� ���, �� ����������� ��������� ����, � � ����� ������ ���
				}
			break;
			}
 
	case 0x58:	// ��� �� ����� ��������� ����, ������� NACK ����� �������� � �����. 
			{
			i2c_Buffer[i2c_index] = TWDR;				// ����� ���� � �����
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// �������� Stop
 
			MACRO_i2c_WhatDo_MasterOut					// ���������� ����� ������
 
			break;
			}
 
// IIC  Slave ============================================================================
 
	case 0x68:	// RCV SLA+W Low Priority				// ������� ���� ����� �� ����� �������� ��������
	case 0x78:	// RCV SLA+W Low Priority (Broadcast)				// ��� ��� ��� ����������������� �����. �� �����
			{
			i2c_Do |= i2c_ERR_LP | i2c_Interrupted;		// ������ ���� ������ Low Priority, � ����� ���� ����, ��� ������� ��������
 
			// Restore Trans after.
			i2c_index = 0;								// ����������� ��������� �������� ������
			i2c_PageAddrIndex = 0;
			}											// � ����� ������. ��������!!! break ��� ���, � ������ ���� � "case 60"
 
	case 0x60: // RCV SLA+W  Incoming?					// ��� ������ �������� ���� �����
	case 0x70: // RCV SLA+W  Incoming? (Broascast)		// ��� ����������������� �����
			{
 
			i2c_Do |= i2c_Busy;							// �������� ����. ����� ������ �� ��������
			i2c_SlaveIndex = 0;							// ��������� �� ������ ������ ������, ������� ����� �����. �� ��������
 
			if (i2c_MasterBytesRX == 1)					// ���� ��� ������� ������� ����� ���� ����, �� ��������� �������  ���
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ������� � ������� ����� ��� �... NACK!
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// � ���� ���� ���� ��� ���� ����, �� ������ � ��������� ��� ACK!
				}
			break;
			}
 
	case 0x80:	// RCV Data Byte						// � ��� �� ������� ���� ����. ��� ��� �����������������. �� �����
	case 0x90:	// RCV Data Byte (Broadcast)
			{
			i2c_InBuff[i2c_SlaveIndex] = TWDR;			// ������� ��� � �����.
 
			i2c_SlaveIndex++;							// �������� ���������
 
			if (i2c_SlaveIndex == i2c_MasterBytesRX-1) 	// �������� ����� ����� ��� ���� ����? 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ������� ��� � ������� NACK!
				}
			else 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ����� ��� ������? ������� � ACK!
				}
			break;
			} 
 
	case 0x88: // RCV Last Byte							// ������� ��������� ����
	case 0x98: // RCV Last Byte (Broadcast)
			{
			i2c_InBuff[i2c_SlaveIndex] = TWDR;			// ������� ��� � �����
 
			if (i2c_Do & i2c_Interrupted)				// ���� � ��� ��� ���������� ����� �� ����� �������
				{
				TWCR = 	1<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ������ � ���� ���� Start �������� � ������� ��� ���� �������
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ���� �� ���� ������ �����, �� ������ ��������� � ����� �����
				}
 
			MACRO_i2c_WhatDo_SlaveOut					// � ������ ���������� ��� �������� ���� ��� ������
			break;
			}
 
 
	case 0xA0: // ��, �� �������� ��������� �����. �� �� ��� � ��� ������? 
			{
 
			// �����, �������, ������� ��������������� �������, ����� ������������ ��� � ������ ���������� �������, ������� ��������. 
			// �� � �� ���� ��������������. � ���� ������ �������� ��� ���.
 
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				1<<TWEA|
				1<<TWEN|
				1<<TWIE;							// ������ �������������, �������������� ���� �����	
			break;
			}
 
 
 
	case 0xB0:  // ������� ���� ����� �� ������ �� ����� �������� ��������			
			{
			i2c_Do |= i2c_ERR_LP | i2c_Interrupted;		// �� ��, ���� ������ � ���� ��������� ��������.
 
 
			// ��������������� �������
			i2c_index = 0;
			i2c_PageAddrIndex = 0;
 
			}											// Break ���! ���� ������						
 
	case 0xA8:	// // ���� ������ ������� ���� ����� �� ������
			{
			i2c_SlaveIndex = 0;							// ������� ��������� �������� �� 0
 
			TWDR = i2c_OutBuff[i2c_SlaveIndex];			// ����, ������� ���� �� ��� ��� ����.
 
			if(i2c_MasterBytesTX == 1)
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// ���� �� ���������, �� ��� �� NACK � ����� �������� 
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// � ���� ���, ��  ACK ����
				}
 
			break;
			} 
 
 
	case 0xB8: // ������� ����, �������� ACK
			{
 
			i2c_SlaveIndex++;								// ������ ���������� ���������. ����� ��������� ����
			TWDR = i2c_OutBuff[i2c_SlaveIndex];				// ���� ��� �������
 
			if (i2c_SlaveIndex == i2c_MasterBytesTX-1)		// ���� �� ��������� ���, ��
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;							// ���� ��� � ���� NACK
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;							// ���� ���, �� ���� � ���� ACK
				}
 
			break;
			}
 
	case 0xC0: // �� ������� ��������� ����, ������ � ��� ���, �������� NACK
	case 0xC8: // ��� ACK. � ������ ������ ��� ���. �.�. ������ ������ � ��� ���.
			{
			if (i2c_Do & i2c_Interrupted)			// ���� ��� ���� ��������� �������� �������
				{									// �� �� ��� �� ������
				i2c_Do &= i2c_NoInterrupted;
													// ������ ���� �����������
				TWCR = 	1<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;					// �������� ����� ����� �� ��� ������� ����.
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;					// ���� �� ��� ����, �� ������ ������� ����
				}
 
			MACRO_i2c_WhatDo_SlaveOut				// � ���������� ����� ������. �������, �� ���
													// �� ����� �� �����. ����� ��� ��� ������, ��� ������
			break;									// ��� ������ ����� �������.
			}
 
	default:	break;
	}
}
 
void DoNothing(void)								// ������� ��������, �������� �������������� ������
{
}
 
void Init_i2c(void)							// ��������� ������ �������
{
i2c_PORT |= 1<<i2c_SCL|1<<i2c_SDA;			// ������� �������� �� ����, ����� ���� �� ��������� ����������
i2c_DDR &= ~(1<<i2c_SCL|1<<i2c_SDA);
 
TWBR = 0x80;         						// �������� �������
TWSR = 0x00;
}
 
void Init_Slave_i2c(IIC_F Addr)				// ��������� ������ ������ (���� �����)
{
TWAR = i2c_MasterAddress;					// ������ � ������� ���� �����, �� ������� ����� ����������. 
											// 1 � ������� ���� ��������, ��� �� ���������� �� ����������������� ������
SlaveOutFunc = Addr;						// �������� ��������� ������ �� ������ ������� ������
 
TWCR = 	0<<TWSTA|
	0<<TWSTO|
	0<<TWINT|
	1<<TWEA|
	1<<TWEN|
	1<<TWIE;							// �������� ������� � �������� ������� ����.
}