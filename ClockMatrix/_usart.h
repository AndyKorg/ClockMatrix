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

//------ Команды
#define CMD_GET_LIST	'L'										//Получить список файлов
#define CMD_PREVIEW		'P'										//Воспроизвести текущий файл
#define CMD_WRITE		'W'										//Записать текущий файл на флэш
#define CMD_OUT			'O'										//Вывести звук из флэш
#define CMD_ABORT		'A'										//Прекратить вывод звука
//------ Ответы
#define RET_HARD_ERROR	'*'										//Непреодолимая проблема
#define RET_SOFT_ERROR	'?'										//Мягкая проблема
#define RET_OK			'0'										//Команда выполнена
#define RET_NO_CARD		'1'										//Нет SD карточки или не поддерживаемая файловая система
#define RET_NO_WAVE		'2'										//Нет файлов типа wav в корне SD карточки
#define RET_BAD_HEAD	'3'										//Не удалось прочитать заголовок файла
#define RET_EMPTY_FLASH	'4'										//Нет записанного файла во флэш памяти.	Возникает при попытке воспроизведения пустой памяти.
#define RET_BAD_CHUNK	'5'										//Плохое описание файла
#define RET_UNKNOWN_CMD	'7'										//Неизвестная команда

//------ конец передачи
#define CMD_END			0x0d									//Символ конца передачи команды или ответа

//------- Настройка USART, перенести в общий каталог солюшена
#define BAUD 9600

#define USART_MODE		(0<<UMSEL)								//Async mode
#define USART_DATA_LEN	((0<<UCSZ2) | (1<<UCSZ1) | (1<<UCSZ0))	//8 Bit data length
#define USART_STOP_BIT	(0<<USBS)								//1 Stop bit
#define USART_PARITY	((0<<UPM1) | (0<<UPM0))					//Parity disabled
#define usartRXIEnable	SetBit(UCSRB, RXCIE)					//Разрешить прерывание от приемника
#define usartRXIEdsable	ClearBit(UCSRB, RXCIE)					//Запретить прерывание от приемника
#define usartTXIEnable	SetBit(UCSRB, UDRIE)					//Разрешить прерывание по окончанию передачи
#define usartTXIEdisable	ClearBit(UCSRB, UDRIE)					//Запретить прерывание по окончанию передачи
#define TXIEisSet(flag)	BitIsSet(flag, UDRIE)					//Прерывания по окончании разрешены?

#include <util/setbaud.h>										//Расчет коэффициентов делителей

//Буфера приема-передачи
#define USART_BUF_BUSY	0x80									//Буфер пуст или занят
#define USART_BUF_READY	0										//Данные в буфере готовы

#define RX_LEN_STR		16										//Длина строки определяется форматом имени файла в DOS 12 = 8+1+3. Плюс небольшой запас

#if (RX_LEN_STR>127)
	#warning "Слишком большой буфер приема"						//Т.к. len в стуктуре буфера 7-й бит используется для флага, длина буфера не может быть больше 127
#endif

struct usartBuf{
	u08 len;
	u08 buf[RX_LEN_STR];
};

#ifndef TX_ONLY
	extern volatile struct usartBuf usartRXbuf;
#endif

extern volatile struct usartBuf usartTXbuf;
#define RxCMDReady		BitIsSet(usartRXbuf.len, 7)				//Седьмой бит в len сообщает о готовности команды в буфере
#define RxCMDReadySet	SetBit(usartRXbuf.len, 7)
#define RxCMDReadyClear	ClearBit(usartRXbuf.len, 7)
#define usartRXAdr		(usartRXbuf.len & 0x7f)					//Маска для адреса в буфере приема

void SerilalIni(void);
unsigned char usart_putchar(char c);
void usart_hex(unsigned char c);
#ifndef TX_ONLY
	unsigned char usart_gets(unsigned char *s);
#endif

#endif /* USART_H_ */