/*
 * ������� � �������� nRF24L01+
 *
 */ 


#ifndef NRF24L01P_REG_H_
#define NRF24L01P_REG_H_

//------------------------ ������� ----------------------------------------
#define	nRF_MASK_REG			0b11100000			//����� ������� ���������� ����������
#define nRF_R_REGISTER			0b00000000			//������ �������
#define nRF_W_REGISTER			0b00100000			//����� � �������
#define nRF_R_RX_PAYLOAD		0b01100001			//���������� �� ������ �������� ������. ������ ������ � �������� ����� FIFO
#define nRF_W_TX_PAYLOAD		0b10100000			//������ ������ � ����� TX_FIFO
#define nRF_FLUSH_TX			0b11100001			//������� TX FIFO
#define nRF_FLUSH_RX			0b11100010			//������� RX FIFO
#define nRF_REUSE_TX_PL			0b11100011			//��������� ��������� ��������. ��������� �������� ������� �� ����� ��������
#define nRF_R_RX_PL_WID			0b01100000			//��������� ���������� ���������� ���� � ������� ������ ������ FIFO, �������� ��������� ��� ������ ������������� ������ ��������
#define nRF_MASK_PIPE			0b00000111			//����� ��� ������ ������
#define nRF_W_ACK_PAYLOAD_MASK	0b10101000			//��������� ��������� ������������ � ������ ������ ������ � ACK �������
#define nRF_W_TX_PAYLOAD_NOACK	0b10110000			//��������� ��������� ��� ����������� ������
#define nRF_NOP					0b11111111			//������ ��������

#define nRF_RD_REG(reg)			((reg & ~nRF_MASK_REG) | nRF_R_REGISTER)	// ������ �������� reg
#define nRF_WR_REG(reg)			((reg & ~nRF_MASK_REG) | nRF_W_REGISTER)	// ������

//------------------------ �������� ----------------------------------------
#define nRF_CONFIG		0x00						//CONFIG - ���������������� �������
#define nRF_MASK_RX_DR  6							//���/���� ���������� �� ���� RX_DR � ���. STATUS. 0-���, 1-����.
#define nRF_MASK_TX_DS  5							//���/���� ���������� �� ���� TX_DS � ���. STATUS. 0-���, 1-����.
#define nRF_MASK_MAX_RT 4							//���/���� ���������� �� ���� MAX_RT � ���. STATUS. 0-���, 1-����.
#define nRF_EN_CRC      3							//��������� CRC. �� ��������� ���. ���� ���� �� ����� �������� EN_AA �������.
#define nRF_CRCO        2							//����� CRC. 0-1 ����, 1-2 �����.
#define nRF_PWR_UP      1							//1-POWER UP, 0-POWER DOWN, �� ��������� 0.
#define nRF_PRIM_RX     0							//0-����� ��������, 1-����� ������.

#define nRF_EN_AA		0x01						//EN_AA - ���������� Enhanced ShockBurst. ���� ���������� ��� ������ �������
#define nRF_ENAA_RESRV1	7							//�� ������������ ������ ������ ���� 0
#define nRF_ENAA_RESRV0	6							//--//--
#define nRF_ENAA_P5		5							//�������� ��������� ��� ������ (pipe) 5
#define nRF_ENAA_P4		4
#define nRF_ENAA_P3		3
#define nRF_ENAA_P2		2
#define nRF_ENAA_P1		1
#define nRF_ENAA_P0		0

#define nRF_EN_RXADDR	0x02						//EN_RXADDR - ���������� ��� RX �������
#define nRF_ERX_RESRV1	7
#define nRF_ERX_RESRV0	6
#define nRF_ERX_P5		5							//��������� ������ ��� ������ 5
#define nRF_ERX_P4		4
#define nRF_ERX_P3		3
#define nRF_ERX_P2		2
#define nRF_ERX_P1		1
#define nRF_ERX_P0		0

#define nRF_SETUP_AW	0x03						//SETUP_AW - ��������� ����� ������, �� 2 �� 7 ���� ��� ���� ������� ������ ���� ������ 0
#define nRF_AW1			1							//����������: 00 - ���������, 01 - 3 �����, 10 - 4 �����, 11 - 1 ����
#define nRF_AW0			0

#define nRF_SETUP_RETR	0x04						//SETUP_RETR - ��������� ����������� ��������
#define nRF_ARD3		7							//� 7-��� �� 4-� ��� �������� ����� ��������
#define nRF_ARD2		6
#define nRF_ARD1		5
#define nRF_ARD0		4
#define nRF_ARC3		3							//� 3-��� �� 0-� ��� ���������� ������������
#define nRF_ARC2		2
#define nRF_ARC1		1
#define nRF_ARC0		0

#define nRF_RF_CH		0x05						//RF_CH - ��������� ������� ������
#define nRF_RF_CH6		6
#define nRF_RF_CH5		5
#define nRF_RF_CH4		4
#define nRF_RF_CH3		3
#define nRF_RF_CH2		2
#define nRF_RF_CH1		1
#define nRF_RF_CH0		0

#define nRF_RF_SETUP	0x06						//RF_SETUP - ��������� �����������, ��������� � ����������, �������� ��������
#define nRF_PLL_LOCK	4
#define nRF_RF_DR		3
#define nRF_RF_PWR1		2
#define nRF_RF_PWR0		1
#define nRF_LNA_HCURR	0

#define nRF_STATUS		0x07						// STATUS - ��������� ����. �������� ������ �� MISO � ����� �� ������ ����
#define nRF_STATUS_BIT7	7							// ������ 0
#define nRF_RX_DR		6							// ����������: ������ ��������. ��� ������ �������� 1.
#define nRF_TX_DS		5							// ����������: ������ ��������. ��� ������ �������� 1.
#define nRF_MAX_RT		4							// ����������: ������ �� ��������. ��� ������ �������� 1.
#define nRF_RX_P_NO2	3							// nRF_RX_P_NO2:nRF_RX_P_NO0 ���� - ����� ������ ������ �� �������� ���� ������� � ����� RX_FIFO
#define nRF_RX_P_NO1	2
#define nRF_RX_P_NO0	1
#define nRF_TX_FULL_ST	0							//���� ������������ TX FIFO ������ ��������. 1-����������, 0-���� ��� �����.


#define nRF_STAT_RESET	((1<<nRF_RX_DR) | (1<<nRF_TX_DS) | (1<<nRF_MAX_RT)) //����� ������ ����������
#define nRF_RX_DATA_RDY(st)		BitIsSet(st, nRF_RX_DR)		//� ������ RX_FIFO ���� ����� �������� ������
#define nRF_TX_DATA_END(st)		BitIsSet(st, nRF_TX_DS)		//�������� ������� ���������. ���� ������� ���������, �� ��������� ������ ����� ��������� ACK �� ���������
#define nRF_TX_ERROR(st)		BitIsSet(st, nRF_MAX_RT)	//������ ��������. ���������� ������� ��������� �������� ����������, � ����� ��� � �� �������. ���������� ���� ��������� ��������
#define nRF_RX_NUM_PIP_MASK		((1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0))	//����� ������������ �� �������� ������ �����
#define nRF_RX_PIP_READY_BIT_LEFT(st)	(st & nRF_RX_NUM_PIP_MASK)	//����� ������ �� �������� ������ ����� ������ �� 1 ��� �����!!!!
#define nRF_RX_IS_EMPTY(st)		(nRF_RX_PIP_READY_BIT_LEFT(st) == nRF_RX_NUM_PIP_MASK) //����� ������ ������
#define nRF_TX_FIFO_FULL(st)	BitIsSet(st, nRF_TX_FULL_ST)//����� ����������� ��������

#define nRF_OBSERVE_TX  0x08						//OBSERVE_TX - ���������� �� ��������� ��������
#define nRF_CD          0x09						//CD - ������� �������, ���������� �������
#define nRF_RX_ADDR_P0  0x0A						//RX_ADDR_P0 - ����� ��� ������ (pipe) 0. �������� �������� - ����� �������� 5 ����
#define nRF_RX_ADDR_P1  0x0B						//����� ��� ������ 1. �������� �������� - ����� �������� 5 ����
#define nRF_RX_ADDR_P2  0x0C						//����� ��� ������ 2. ����� �������� 1 ����, ������� ����� ��������� ����� RX_ADDR_P1
#define nRF_RX_ADDR_P3  0x0D						//����� ��� ������ 3. ����� �������� 1 ����, ������� ����� ��������� ����� RX_ADDR_P1
#define nRF_RX_ADDR_P4  0x0E						//����� ��� ������ 4. ����� �������� 1 ����, ������� ����� ��������� ����� RX_ADDR_P1
#define nRF_RX_ADDR_P5  0x0F						//����� ��� ������ 5. ����� �������� 1 ����, ������� ����� ��������� ����� RX_ADDR_P1
#define nRF_TX_ADDR     0x10						//����� ������ ��������. ������������� ������ � ������ PTX. �������� �������� - ����� �������� 5 ����
#define nRF_RX_PW_P0	0x11						//���������� ����������� ���� ��� ������ 0, �� 1 �� 32. � ������������ ������ �� ������������
#define nRF_RX_PW_P1	0x12
#define nRF_RX_PW_P2	0x13
#define nRF_RX_PW_P3	0x14
#define nRF_RX_PW_P4	0x15
#define nRF_RX_PW_P5	0x16

#define nRF_FIFO_STATUS 0x17						//FIFO_STATUS - ��������� FIFO
#define nRF_TX_REUSE	6							
#define nRF_TX_FULL		5							
#define nRF_TX_EMPTY	4
#define nRF_RX_FULL		1
#define nRF_RX_EMPTY	0

#define nRF_DYNPD		0x1C						//DYNPD - ��������� ������������ ��������� �������� ������ ��� �������
#define nRF_DPL_P5		5
#define nRF_DPL_P4		4
#define nRF_DPL_P3		3
#define nRF_DPL_P2		2
#define nRF_DPL_P1		1
#define nRF_DPL_P0		0

#define nRF_FEATURE		0x1D						//FEATURE - ��������������. ������������ ������ ��� ����.
#define nRF_EN_DPL		2							//������������ ��������� ����� �������
#define nRF_EN_ACK_PAY	1							//��������� ���������� � ����� ACK ���������������� ������
#define nRF_EN_DYN_ACK	0							//��������� �������� ��� �������������

#endif /* NRF24L01P_REG_H_ */