/*
		Автор DI HALT (http://easyelectronics.ru)
		Язык: C
		Опубликовано 23 ноября 2010 года в 22:56
		Хидер для библиотеки IIC
*/

#ifndef IICULTIMATE_H
#define IICULTIMATE_H
  
#include <avr/io.h>
#include <avr/interrupt.h>
#include "avrlibdefs.h"
#include "avrlibtypes.h"
 
//(Platform dependent)
#define i2c_PORT	PORTC		// Порт где сидит нога TWI
#define i2c_DDR		DDRC
#define i2c_SCL		PORTC5		// Биты соответствующих выводов
#define i2c_SDA		PORTC4
 
 
#define i2c_MasterAddress 	1 //0x32	// Адрес на который будем отзываться
#define i2c_i_am_slave		0			// Если мы еще и слейвом работаем то 1. А то не услышит!
 
#define i2c_MasterBytesRX	0x0			// Величина принимающего буфера режима Slave, т.е. сколько байт жрем.
#define i2c_MasterBytesTX	0x0			// Величина Передающего буфера режима Slave , т.е. сколько байт отдаем за сессию.
 
#define i2c_MaxBuffer		0x12		// Величина буфера Master режима RX-TX. Зависит от того какой длины строки мы будем гонять
#define i2c_MaxPageAddrLgth	1			// Максимальная величина адреса страницы. Обычно адрес страницы это один или два байта. 
										// Зависит от типа ЕЕПРОМ или другой микросхемы. 
  
#define i2c_type_msk	0b00001100	// Маска режима
#define i2c_sarp	0b00000000	// Start-Addr_R-Read-Stop  							Это режим простого чтения. Например из слейва или из епрома с текущего адреса
#define i2c_sawp	0b00000100	// Start-Addr_W-Write-Stop 							Это режим простой записи. В том числе и запись с адресом страницы. 
#define i2c_sawsarp	0b00001000	// Start-Addr_W-WrPageAdr-rStart-Addr_R-Read-Stop 	Это режим с предварительной записью текущего адреса страницы
 
#define i2c_Err_msk	0b00110011	// Маска кода ошибок
#define i2c_Err_NO	0b00000000	// All Right! 						-- Все окей, передача успешна. 
#define i2c_ERR_NA	0b00010000	// Device No Answer 				-- Слейв не отвечает. Т.к. либо занят, либо его нет на линии.
#define i2c_ERR_LP	0b00100000	// Low Priority 					-- нас перехватили собственным адресом, либо мы проиграли арбитраж
#define i2c_ERR_NK	0b00000010	// Received NACK. End Transmittion. -- Был получен NACK. Бывает и так.
#define i2c_ERR_BF	0b00000001	// BUS FAIL 						-- Автобус сломался. И этим все сказано. Можно попробовать сделать переинициализацию шины
 
#define i2c_Interrupted		0b10000000		// Transmiting Interrupted		Битмаска установки флага занятости
#define i2c_NoInterrupted 	0b01111111  	// Transmiting No Interrupted	Битмаска снятия флага занятости
 
#define i2c_Busy		0b01000000  	// Trans is Busy				Битмаска флага "Передатчик занят, руками не трогать". 
#define i2c_Free		0b10111111  	// Trans is Free				Битмаска снятия флага занятости.
 
 
#define MACRO_i2c_WhatDo_MasterOut 	(MasterOutFunc)();		// Макросы для режимо выхода. Пока тут функция, но может быть что угодно
#define MACRO_i2c_WhatDo_SlaveOut   	(SlaveOutFunc)();
#define MACRO_i2c_WhatDo_ErrorOut   	(ErrorOutFunc)();
 
 
 
typedef void (*IIC_F)(void);		// Тип указателя на функцию
 
extern IIC_F MasterOutFunc;			// Подробности в сишнике. 
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