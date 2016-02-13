/*
* ����� ����������� �� ������������
*/

#include "nRF24L01P.h"
#include "EERTOS.h"
#include "sensors.h"
#include "Sound.h"

/************************************************************************/
/* ����- ����� ����� �� SPI. ���������� �������� ����������� �� MISO    */
/************************************************************************/
u08 nRF_ExchangeSPI(u08 Value){
	u08 Ret = 0;

	for(u08 i=8;i;i--){
		Ret <<= 1;
		if BitIsSet(nRF_PORTIN, nRF_MISO)				//������ MISO
			Ret |= 1;
		ClearBit(nRF_PORT, nRF_MOSI);
		if (Value & 0x80)								//����� MOSI
			SetBit(nRF_PORT, nRF_MOSI);
		Value <<= 1;
		SetBit(nRF_PORT, nRF_SCK);						//�������� ���
		ClearBit(nRF_PORT, nRF_SCK);
	}
	return Ret;
}

/************************************************************************/
/* �������� �������. ���������� ������ ������							*/
/* cmd - ��� �������,													*/
/* Len - ����� ������ � ������ �� ������� ��������� Data				*/
/************************************************************************/
u08 nRF_cmd_Write(const u08 cmd, u08 Len, u08 *Data){
	u08 Status;
	
	nRF_SELECT();
	Status = nRF_ExchangeSPI(cmd);
	if (Len){
		for(u08 i=0; Len; Len--, i++)
			nRF_ExchangeSPI(*(Data+i));
	}
	nRF_DESELECT();
	return Status;
}

/************************************************************************/
/* ����� ������ �� nRF24L01P											*/
/* Len - ���������� ���� ��������� ������. 								*/
/* ���������� ��������� �������� ���� ��� ������ ������ ���� len=0 (	*/
/* ������� ����� Buf ��� �� ����� ������ ������							*/
/************************************************************************/
u08 nRF_cmd_Read(const u08 Cmd, u08 Len, u08 *Data){
	u08 Ret;
	nRF_SELECT();
	Ret = nRF_ExchangeSPI(Cmd);
	if (Len){
		for(u08 i=0; Len; Len--, i++){
			*(Data+i) = nRF_ExchangeSPI(nRF_NOP);
			Ret = *(Data+i);
		}
	}
	else
		*Data = Ret;
	nRF_DESELECT();
	return Ret;
}
/************************************************************************/
/* �������� ��������� ������ �� ������������                            */
/************************************************************************/
void nRF_Recive(void){
	u08 Buf[nRF_SEND_LEN], Status, CountByts;
	struct sSensor Sens;								//���� ���� ���������, ��� �� ���� �����, ��� ����� �������� �� �������� �������
	
	#define nRF_NoAnswer()	Sens.SleepPeriod = 0		//��� ������ �� ����������� ��� ����� �� ���
	#define nRF_AnswerOk()	Sens.SleepPeriod = 0xff		//������� ����� �� �����������
	#define nRF_AnswerIsOk() (Sens.SleepPeriod)
	
#ifdef nRF_IRQ											//�������� ���� ����������
	if (nRF_NO_RECIV()){								//������ �� �������
		SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
		return;
	}
#else													//���� ���������� �� ��������, ����� ����� ������
	Status = nRF_cmd_Read(nRF_RD_REG(nRF_STATUS), 0, Buf);
	if ((Status & nRF_STAT_RESET) == 0){				//������ �� �������
		SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
		return;
	}
#endif

	Sens.State = 0;
	SensorNoInBus(Sens);								//������ �� ������������ ��� ���
	Sens.Value = 0;										//��� ��� �� ���������� �� ��������� ��������������
	nRF_NoAnswer();
	nRF_STOP();											//���������� ������ �����������

	Buf[0] = Status & nRF_STAT_RESET;					//�������� ����� �������
	nRF_cmd_Write(nRF_WR_REG(nRF_STATUS), 1, Buf);

	if nRF_RX_DATA_RDY(Status){							//���-�� �������

		CountByts = nRF_cmd_Read(nRF_RD_REG(nRF_R_RX_PL_WID), 1, Buf);
		if (CountByts > nRF_SEND_LEN)					//TODO: �� ���������� ������� ����������� 11 ���� ������ 4
			CountByts = nRF_SEND_LEN;
		
		if (CountByts == nRF_SEND_LEN){					//������ ������ ����������
			nRF_AnswerOk();
			nRF_cmd_Read(nRF_R_RX_PAYLOAD, CountByts, Buf);
			nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);		//�������� ������ FIFO
			
			if ((Buf[0] == nRF_RESERVED_BYTE) && (Buf[1] == nRF_TMPR_ATTNY13_SENSOR)){	//���������� �����
				SensorSetInBus(Sens);													//���� ����� �� ������������
				if ((((u16)Buf[2]<<8) | ((u16)Buf[3])) == nRF_SENSOR_NO)				//������ �� ���� �� ���������
					Sens.Value = TMPR_NO_SENS;											//��� ������ �� ��������
				else
					Sens.Value = ( ((Buf[2]<<4) & 0xf0) | ((Buf[3]>>4) & 0x0f) );		//�������� �����������
				if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite)			//���� ����� �������� ������ ����� �������, �� ��������� �������� ����� 1 �������
					&& (SensorNum(CurrentSensorShow)->Adr == nRF_RX_PIP_READY_BIT_LEFT(Status))
					)
					{				
					Buf[0] = ATTINY_13A_1S_SLEEP;										//������� ��� ������� ���
					Buf[1] = 0x00;														//������� ���� �������� ���
					Buf[2] = nRF_MEASURE_TEST_SEC;										//������� ���� ��������
				}
				else{																	//���� ���������� ����� �� ��������� �������� ����� 20 �����	
					Buf[0] = ATTINY_13A_8S_SLEEP;										//������� ��� ������� ���
					Buf[1] = nRF_INTERVAL_NORM_MSB;										//������� ���� �������� ���
					Buf[2] = nRF_INTERVAL_NORM_LSB;										//������� ���� ��������
				}
				nRF_cmd_Write(nRF_FLUSH_TX, 0, Buf);									//�������� ����� ��������
				nRF_cmd_Write((nRF_W_ACK_PAYLOAD_MASK | (nRF_RX_PIP_READY_BIT_LEFT(Status)>>1)), nRF_ACK_LEN, Buf);	//�������� ����� ��� ������
			}
		}
		else{																			//������ ������������ �����, ������� ����� ������
			nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);
		}
	}

	if nRF_AnswerIsOk(){
		SetSensor(nRF_RX_PIP_READY_BIT_LEFT(Status), Sens.State, Sens.Value);	//�������� ������ � ������ ��������. ����� ������� �������. ��������� ��. � ���������� SetSensor
		if ((ClockStatus == csSensorSet) && (SetStatus == ssSensWaite)){		//���� ����� �������� �������, �� ����� �������� �����
			Refresh();
		}
	}

	SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);
	nRF_GO();											//���������� �����
}

/************************************************************************/
/* ������������� nRF24L01												*/
/************************************************************************/
void nRF_Init(void){
	u08 Buf[nRF_ACK_LEN], Status;

	nRF_DDR |=	(1<<nRF_MOSI)	|
				(0<<nRF_MISO)	|
				(1<<nRF_SCK)	|
				#ifdef nRF_CE
				(1<<nRF_CE)		|						//������ ������
				#endif
				#ifdef nRF_IRQ
				(1<<nRF_IRQ)	|
				#endif
				(1<<nRF_CSN);							//����� ������
	nRF_PORT |= (1<<nRF_MISO);							//��������� ���� � ������� �� ������ ������

	nRF_STOP();											//����� ���������
	nRF_DESELECT();										//������ �� ������
	nRF_cmd_Write(nRF_FLUSH_RX, 0, Buf);				//��������� ������� ������ �� SPI. ������� �������� ������ RX � TX FIFO
	nRF_cmd_Write(nRF_FLUSH_TX, 0, Buf);
	Status = nRF_cmd_Read(nRF_WR_REG(nRF_STATUS), 0, Buf);

	Buf[0] &=	(1<<nRF_STATUS_BIT7) |					//���� ��� ������ ���� ������ 0
				(1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0) | //��� ���� ����� ������� ������ RX FIFO ������ ���� ����� 111 (����� RX FIFO ����)
				(1<<nRF_TX_FULL_ST);					//����� ������� ������ TX FIFO ������ ���� ����� 0 (� ������ TX FIFO ���� �����)
	
	if (Buf[0] != ((0<<nRF_STATUS_BIT7) |				//���� ��� ������ ���� ������ 0
				(1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0) | //��� ���� ����� ������� ������ RX FIFO ������ ���� ����� 111 (����� RX FIFO ����)
				(0<<nRF_TX_FULL_ST))					//����� ������� ������ TX FIFO ������ ���� ����� 0 (� ������ TX FIFO ���� �����)
		){												//���� ��� �� ��� ������������� ������ ��� � ������� �������
		return;											//������ ���, ������� �������� �� ����������
	}

	Buf[0] = Status & nRF_STAT_RESET;					//�������� ����� ���������� �� ������ ������
	nRF_cmd_Write(nRF_WR_REG(nRF_STATUS), 1, Buf);
	
	Buf[0] = (1<<nRF_ERX_P5) | (1<<nRF_ERX_P4) | (1<<nRF_ERX_P3) | (1<<nRF_ERX_P2) | (1<<nRF_ERX_P1) | (1<<nRF_ERX_P0);
	nRF_cmd_Write(nRF_WR_REG(nRF_EN_RXADDR), 1, Buf);	//��������� ���������� ��������� ���� ������

	Buf[0] = (1<<nRF_EN_DPL) | (1<<nRF_EN_ACK_PAY);		//��������� ������������ ����� ������� � ������ ������
	nRF_cmd_Write(nRF_WR_REG(nRF_FEATURE), 1, Buf);

	Buf[0] = (1<<nRF_DPL_P5) | (1<<nRF_DPL_P4) | (1<<nRF_DPL_P3) | (1<<nRF_DPL_P2) | (1<<nRF_DPL_P1) | (1<<nRF_DPL_P0);//��������� ������������ ����� ������� ��� ���� �������
	nRF_cmd_Write(nRF_WR_REG(nRF_DYNPD), 1, Buf);

	Buf[0] = (1<<nRF_PRIM_RX) | (1<<nRF_PWR_UP) | (1<<nRF_EN_CRC);	//����� ������
	nRF_cmd_Write(nRF_WR_REG(nRF_CONFIG), 1, Buf);

	Buf[0] = ATTINY_13A_1S_SLEEP;						//������� ��� ������� ���
	Buf[1] = nRF_INTERVAL_NORM_MSB;						//������� ���� �������� ���
	Buf[2] = nRF_INTERVAL_NORM_LSB;						//������� ���� ��������
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);	//�������� ����� ��� ������ nRF_PIPE
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);
	nRF_cmd_Write(nRF_W_ACK_PAYLOAD_MASK | nRF_PIPE, nRF_ACK_LEN, Buf);

	SetTimerTask(nRF_Recive, nRF_PERIOD_TEST);			//�������� ����� ������
	nRF_GO();											//������ �����
}