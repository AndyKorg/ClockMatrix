/*
 * usart.c
 *
 * Created: 21.10.2013 13:47:36
 *  Author: Admin
 */ 

#include "usart.h"
#include <string.h>
#include <stddef.h>
//#include "sensors.h"

#ifdef DEBUG

#ifndef TX_ONLY
	volatile struct usartBuf usartRXbuf;
#endif

volatile struct usartBuf usartTXbuf;

/************************************************************************/
/* �����-�������� USART													*/
/************************************************************************/
//-------- ������������� USART
void SerilalIni(void){
	UCSRB =	(1<<RXEN) |											//��������� �����
			(1<<TXEN);											//��������� �������� �� USART
	usartRXIEnable;												//��������� ���������� �� ���������, ��� ������ �������� ������������� ����������� ���������� ��� ������ �������� �����������
	UCSRC = USART_MODE | USART_DATA_LEN | USART_STOP_BIT | USART_PARITY;
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X
		UCSRA |= (1 << U2X);
	#else
		UCSRA &= ~(0 << U2X);
	#endif
#ifndef TX_ONLY
	usartRXbuf.len = 0;
#endif
	usartTXbuf.len = 0;
}

#ifndef TX_ONLY
//------ ���������� ����� ����� � ��������� ��� � �����
ISR(USART_RXC_vect){
	u08 rxbyte = UDR;
	
	if RxCMDReady												//���������� ������� �� ����������, ������������ �������
		return;
	if (usartRXAdr != RX_LEN_STR){
		usartRXbuf.buf[usartRXAdr] = rxbyte;
		usartRXbuf.len++;
		UDR = rxbyte;
		usartTXIEnable;											//����������� ��������
		//RxCMDReadySet;
	}
}
#endif

//------ ���������� - ���������� ����
ISR(USART_UDRE_vect){
	static u08 CurrentChar = 0;
	
	if (usartTXbuf.len){
		UDR = usartTXbuf.buf[CurrentChar++];					//���������� ��������� ������
		if (usartTXbuf.len == CurrentChar){						//������ � ������ ������ ���, �������� ���������
			usartTXIEdisable;
			CurrentChar = 0;
			usartTXbuf.len = 0;									//��� ��������
		}
	}
}

//------ �������� ���� c � ����� ��������
unsigned char usart_putchar(char c){
	u08 storeInt = UCSRB, Ret = USART_BUF_READY;				//������ ������������ ������� � �����
	
	usartTXIEdisable;											//��������� ���������� �� ����������� �� ����� ���������
	if (usartTXbuf.len == RX_LEN_STR)							//����� �������� �����
		Ret = USART_BUF_BUSY;
	else
		usartTXbuf.buf[usartTXbuf.len++] = c;
	if TXIEisSet(storeInt)										//���� ���������� ���� ��������� �� ��������� �� �����
		usartTXIEnable;
	usartTXIEnable;												//����������� ��������
	return Ret;
}

void usart_hex(unsigned char c){
	#define HI(b)	((b>>4) & 0xf)
	#define LO(b)	(b & 0xf)
	#define HE(b)	(((b & 0x7)-1) | 0x40)
	
	if (HI(c)>9)
		usart_putchar(HE(HI(c)));
	else
		usart_putchar(0x30 | HI(c));
	if (LO(c)>9)
		usart_putchar(HE(LO(c)));
	else
		usart_putchar(0x30 | LO(c));
}

/************************************************************************/
/* ���������� ���������� �������� ���� ��� 0 ���� ������ �� �������     */
/************************************************************************/
#ifndef TX_ONLY
unsigned char usart_gets(unsigned char *s){
	u08 len = usartRXAdr;
	
	if RxCMDReady{
		memcpy(s, &usartRXbuf.buf, len);
		usartRXbuf.len = 0;
		RxCMDReadyClear;
		return len;
	}
	else
		return 0;
}
#endif

#endif //#ifdef DEBUG
