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
/* Прием-передача USART													*/
/************************************************************************/
//-------- Инициализация USART
void SerilalIni(void){
	UCSRB =	(1<<RXEN) |											//Разрешить прием
			(1<<TXEN);											//Разрешить передачу по USART
	usartRXIEnable;												//Разрешить прерывание от приемника, при начале передачи дополнительно разрешается прерывания при пустом регистре передатчика
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
//------ Собственно прием байта и помещение его в буфер
ISR(USART_RXC_vect){
	u08 rxbyte = UDR;
	
	if RxCMDReady												//Предыдущая команда не обработана, игнорируется текущая
		return;
	if (usartRXAdr != RX_LEN_STR){
		usartRXbuf.buf[usartRXAdr] = rxbyte;
		usartRXbuf.len++;
		UDR = rxbyte;
		usartTXIEnable;											//Разрешается передача
		//RxCMDReadySet;
	}
}
#endif

//------ Прерывание - передатчик пуст
ISR(USART_UDRE_vect){
	static u08 CurrentChar = 0;
	
	if (usartTXbuf.len){
		UDR = usartTXbuf.buf[CurrentChar++];					//Передается очередной символ
		if (usartTXbuf.len == CurrentChar){						//Данных в буфере больше нет, передачу запрещаем
			usartTXIEdisable;
			CurrentChar = 0;
			usartTXbuf.len = 0;									//Все передано
		}
	}
}

//------ Помещает байт c в буфер передачи
unsigned char usart_putchar(char c){
	u08 storeInt = UCSRB, Ret = USART_BUF_READY;				//Символ благополучно помещен в буфер
	
	usartTXIEdisable;											//Запретить прерывание от передатчика на время обработки
	if (usartTXbuf.len == RX_LEN_STR)							//Буфер передачи полон
		Ret = USART_BUF_BUSY;
	else
		usartTXbuf.buf[usartTXbuf.len++] = c;
	if TXIEisSet(storeInt)										//Если прерывания были разрешены то разрешить их снова
		usartTXIEnable;
	usartTXIEnable;												//Разрешается передача
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
/* Возвращает количество принятых байт или 0 если ничего не принято     */
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
