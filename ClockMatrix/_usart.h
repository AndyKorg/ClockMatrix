/*
 * usart.h
 *
 * Created: 21.10.2013 13:47:19
 *  Author: Admin
 */ 


#ifndef USART_H_
#define USART_H_

#define TX_ONLY

#ifndef F_CPU
	#warning "F_CPU not define"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>

#include "avrlibtypes.h"
#include "bits_macros.h"

//------ �������
#define CMD_GET_LIST	'L'										//�������� ������ ������
#define CMD_PREVIEW		'P'										//������������� ������� ����
#define CMD_WRITE		'W'										//�������� ������� ���� �� ����
#define CMD_OUT			'O'										//������� ���� �� ����
#define CMD_ABORT		'A'										//���������� ����� �����
//------ ������
#define RET_HARD_ERROR	'*'										//������������� ��������
#define RET_SOFT_ERROR	'?'										//������ ��������
#define RET_OK			'0'										//������� ���������
#define RET_NO_CARD		'1'										//��� SD �������� ��� �� �������������� �������� �������
#define RET_NO_WAVE		'2'										//��� ������ ���� wav � ����� SD ��������
#define RET_BAD_HEAD	'3'										//�� ������� ��������� ��������� �����
#define RET_EMPTY_FLASH	'4'										//��� ����������� ����� �� ���� ������.	��������� ��� ������� ��������������� ������ ������.
#define RET_BAD_CHUNK	'5'										//������ �������� �����
#define RET_UNKNOWN_CMD	'7'										//����������� �������

//------ ����� ��������
#define CMD_END			0x0d									//������ ����� �������� ������� ��� ������

//------- ��������� USART, ��������� � ����� ������� ��������
#define BAUD 9600

#define USART_MODE		(0<<UMSEL)								//Async mode
#define USART_DATA_LEN	((0<<UCSZ2) | (1<<UCSZ1) | (1<<UCSZ0))	//8 Bit data length
#define USART_STOP_BIT	(0<<USBS)								//1 Stop bit
#define USART_PARITY	((0<<UPM1) | (0<<UPM0))					//Parity disabled
#define usartRXIEnable	SetBit(UCSRB, RXCIE)					//��������� ���������� �� ���������
#define usartRXIEdsable	ClearBit(UCSRB, RXCIE)					//��������� ���������� �� ���������
#define usartTXIEnable	SetBit(UCSRB, UDRIE)					//��������� ���������� �� ��������� ��������
#define usartTXIEdisable	ClearBit(UCSRB, UDRIE)					//��������� ���������� �� ��������� ��������
#define TXIEisSet(flag)	BitIsSet(flag, UDRIE)					//���������� �� ��������� ���������?

#include <util/setbaud.h>										//������ ������������� ���������

//������ ������-��������
#define USART_BUF_BUSY	0x80									//����� ���� ��� �����
#define USART_BUF_READY	0										//������ � ������ ������

#define RX_LEN_STR		16										//����� ������ ������������ �������� ����� ����� � DOS 12 = 8+1+3. ���� ��������� �����

#if (RX_LEN_STR>127)
	#warning "������� ������� ����� ������"						//�.�. len � �������� ������ 7-� ��� ������������ ��� �����, ����� ������ �� ����� ���� ������ 127
#endif

struct usartBuf{
	u08 len;
	u08 buf[RX_LEN_STR];
};

#ifndef TX_ONLY
	extern volatile struct usartBuf usartRXbuf;
#endif

extern volatile struct usartBuf usartTXbuf;
#define RxCMDReady		BitIsSet(usartRXbuf.len, 7)				//������� ��� � len �������� � ���������� ������� � ������
#define RxCMDReadySet	SetBit(usartRXbuf.len, 7)
#define RxCMDReadyClear	ClearBit(usartRXbuf.len, 7)
#define usartRXAdr		(usartRXbuf.len & 0x7f)					//����� ��� ������ � ������ ������

void SerilalIni(void);
unsigned char usart_putchar(char c);
void usart_hex(unsigned char c);
#ifndef TX_ONLY
	unsigned char usart_gets(unsigned char *s);
#endif

#endif /* USART_H_ */