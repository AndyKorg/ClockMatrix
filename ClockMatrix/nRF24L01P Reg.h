/*
 * Команды и регистры nRF24L01+
 *
 */ 


#ifndef NRF24L01P_REG_H_
#define NRF24L01P_REG_H_

//------------------------ Команды ----------------------------------------
#define	nRF_MASK_REG			0b11100000			//Маска команды управления регистрами
#define nRF_R_REGISTER			0b00000000			//читаем регистр
#define nRF_W_REGISTER			0b00100000			//пишем в регистр
#define nRF_R_RX_PAYLOAD		0b01100001			//считывание из буфера принятых данных. Читаем всегда с нулевого байта FIFO
#define nRF_W_TX_PAYLOAD		0b10100000			//запись данных в буфер TX_FIFO
#define nRF_FLUSH_TX			0b11100001			//очистка TX FIFO
#define nRF_FLUSH_RX			0b11100010			//очистка RX FIFO
#define nRF_REUSE_TX_PL			0b11100011			//повторить последнюю передачу. ЗАПРЕЩЕНО выдавать команду во время передачи
#define nRF_R_RX_PL_WID			0b01100000			//Прочитать количество полученных байт в верхнем буфере приема FIFO, особенно актуально для режима динамического обмена пакетами
#define nRF_MASK_PIPE			0b00000111			//Маска для номера канала
#define nRF_W_ACK_PAYLOAD_MASK	0b10101000			//Загрузить сообщение передаваемое в режиме приема вместе с ACK пакетом
#define nRF_W_TX_PAYLOAD_NOACK	0b10110000			//Отключить автоответ для конкретного пакета
#define nRF_NOP					0b11111111			//Пустая операция

#define nRF_RD_REG(reg)			((reg & ~nRF_MASK_REG) | nRF_R_REGISTER)	// Чтение регистра reg
#define nRF_WR_REG(reg)			((reg & ~nRF_MASK_REG) | nRF_W_REGISTER)	// запись

//------------------------ Регистры ----------------------------------------
#define nRF_CONFIG		0x00						//CONFIG - Конфигурационный регистр
#define nRF_MASK_RX_DR  6							//вкл/откл прерывание от бита RX_DR в рег. STATUS. 0-вкл, 1-выкл.
#define nRF_MASK_TX_DS  5							//вкл/откл прерывание от бита TX_DS в рег. STATUS. 0-вкл, 1-выкл.
#define nRF_MASK_MAX_RT 4							//вкл/откл прерывание от бита MAX_RT в рег. STATUS. 0-вкл, 1-выкл.
#define nRF_EN_CRC      3							//включение CRC. По умолчанию вкл. если один из битов регистра EN_AA включен.
#define nRF_CRCO        2							//режим CRC. 0-1 байт, 1-2 байта.
#define nRF_PWR_UP      1							//1-POWER UP, 0-POWER DOWN, по умолчанию 0.
#define nRF_PRIM_RX     0							//0-режим передачи, 1-режим приема.

#define nRF_EN_AA		0x01						//EN_AA - технология Enhanced ShockBurst. Ржим автоответа для разных каналов
#define nRF_ENAA_RESRV1	7							//Не используется всегда должно быть 0
#define nRF_ENAA_RESRV0	6							//--//--
#define nRF_ENAA_P5		5							//Включить автоответ для канала (pipe) 5
#define nRF_ENAA_P4		4
#define nRF_ENAA_P3		3
#define nRF_ENAA_P2		2
#define nRF_ENAA_P1		1
#define nRF_ENAA_P0		0

#define nRF_EN_RXADDR	0x02						//EN_RXADDR - Разрешение для RX адресов
#define nRF_ERX_RESRV1	7
#define nRF_ERX_RESRV0	6
#define nRF_ERX_P5		5							//Разрешить данные для канала 5
#define nRF_ERX_P4		4
#define nRF_ERX_P3		3
#define nRF_ERX_P2		2
#define nRF_ERX_P1		1
#define nRF_ERX_P0		0

#define nRF_SETUP_AW	0x03						//SETUP_AW - Установка длины адреса, со 2 по 7 биты для всех каналов должны быть всегда 0
#define nRF_AW1			1							//Комбинации: 00 - запрещена, 01 - 3 байта, 10 - 4 байта, 11 - 1 байт
#define nRF_AW0			0

#define nRF_SETUP_RETR	0x04						//SETUP_RETR - Настройка автоповтора передачи
#define nRF_ARD3		7							//с 7-ого по 4-й бит задержка перед повтором
#define nRF_ARD2		6
#define nRF_ARD1		5
#define nRF_ARD0		4
#define nRF_ARC3		3							//с 3-ого по 0-й бит количество автоповторов
#define nRF_ARC2		2
#define nRF_ARC1		1
#define nRF_ARC0		0

#define nRF_RF_CH		0x05						//RF_CH - Настройка частоты канала
#define nRF_RF_CH6		6
#define nRF_RF_CH5		5
#define nRF_RF_CH4		4
#define nRF_RF_CH3		3
#define nRF_RF_CH2		2
#define nRF_RF_CH1		1
#define nRF_RF_CH0		0

#define nRF_RF_SETUP	0x06						//RF_SETUP - настройка радиоканала, включение и выключение, скорость передачи
#define nRF_PLL_LOCK	4
#define nRF_RF_DR		3
#define nRF_RF_PWR1		2
#define nRF_RF_PWR0		1
#define nRF_LNA_HCURR	0

#define nRF_STATUS		0x07						// STATUS - состояние чипа. Выдается всегда на MISO в ответ на первый байт
#define nRF_STATUS_BIT7	7							// Всегда 0
#define nRF_RX_DR		6							// прерывание: данные получены. Для сброса записать 1.
#define nRF_TX_DS		5							// прерывание: данные переданы. Для сброса записать 1.
#define nRF_MAX_RT		4							// прерывание: данные не переданы. Для сброса записать 1.
#define nRF_RX_P_NO2	3							// nRF_RX_P_NO2:nRF_RX_P_NO0 биты - номер канала данные по которому были приняты в буфер RX_FIFO
#define nRF_RX_P_NO1	2
#define nRF_RX_P_NO0	1
#define nRF_TX_FULL_ST	0							//флаг переполнения TX FIFO буфера передачи. 1-переполнен, 0-есть еще место.


#define nRF_STAT_RESET	((1<<nRF_RX_DR) | (1<<nRF_TX_DS) | (1<<nRF_MAX_RT)) //Сброс фалгов прерываний
#define nRF_RX_DATA_RDY(st)		BitIsSet(st, nRF_RX_DR)		//В буфере RX_FIFO есть новые принятые данные
#define nRF_TX_DATA_END(st)		BitIsSet(st, nRF_TX_DS)		//Передача успешно закончена. Если включен автоответ, то взводится только после получения ACK от приемника
#define nRF_TX_ERROR(st)		BitIsSet(st, nRF_MAX_RT)	//Ошибка передачи. Количество попыток повторной передачи достигнуто, а ответ так и не получен. Взведенный флаг запрещает передачи
#define nRF_RX_NUM_PIP_MASK		((1<<nRF_RX_P_NO2) | (1<<nRF_RX_P_NO1) | (1<<nRF_RX_P_NO0))	//Маска номераканала по которому принят пакет
#define nRF_RX_PIP_READY_BIT_LEFT(st)	(st & nRF_RX_NUM_PIP_MASK)	//Номер канала по которому принят пакет сдвинт на 1 бит влево!!!!
#define nRF_RX_IS_EMPTY(st)		(nRF_RX_PIP_READY_BIT_LEFT(st) == nRF_RX_NUM_PIP_MASK) //Буфер приема пустой
#define nRF_TX_FIFO_FULL(st)	BitIsSet(st, nRF_TX_FULL_ST)//Буфер передатчика заполнен

#define nRF_OBSERVE_TX  0x08						//OBSERVE_TX - наблюдение за процессом передачи
#define nRF_CD          0x09						//CD - принята несущая, устаревший регистр
#define nRF_RX_ADDR_P0  0x0A						//RX_ADDR_P0 - адрес для канала (pipe) 0. Обратите внимание - длина регистра 5 байт
#define nRF_RX_ADDR_P1  0x0B						//адрес для канала 1. Обратите внимание - длина регистра 5 байт
#define nRF_RX_ADDR_P2  0x0C						//адрес для канала 2. Длина регистра 1 байт, старшие байты повторяют байты RX_ADDR_P1
#define nRF_RX_ADDR_P3  0x0D						//адрес для канала 3. Длина регистра 1 байт, старшие байты повторяют байты RX_ADDR_P1
#define nRF_RX_ADDR_P4  0x0E						//адрес для канала 4. Длина регистра 1 байт, старшие байты повторяют байты RX_ADDR_P1
#define nRF_RX_ADDR_P5  0x0F						//адрес для канала 5. Длина регистра 1 байт, старшие байты повторяют байты RX_ADDR_P1
#define nRF_TX_ADDR     0x10						//Адрес канала передачи. Действительно только в режиме PTX. Обратите внимание - длина регистра 5 байт
#define nRF_RX_PW_P0	0x11						//Количество принимаемых байт для канала 0, от 1 до 32. В динамическом режиме не используется
#define nRF_RX_PW_P1	0x12
#define nRF_RX_PW_P2	0x13
#define nRF_RX_PW_P3	0x14
#define nRF_RX_PW_P4	0x15
#define nRF_RX_PW_P5	0x16

#define nRF_FIFO_STATUS 0x17						//FIFO_STATUS - состояние FIFO
#define nRF_TX_REUSE	6							
#define nRF_TX_FULL		5							
#define nRF_TX_EMPTY	4
#define nRF_RX_FULL		1
#define nRF_RX_EMPTY	0

#define nRF_DYNPD		0x1C						//DYNPD - разрешить динамическое изменение принятых данных для каналов
#define nRF_DPL_P5		5
#define nRF_DPL_P4		4
#define nRF_DPL_P3		3
#define nRF_DPL_P2		2
#define nRF_DPL_P1		1
#define nRF_DPL_P0		0

#define nRF_FEATURE		0x1D						//FEATURE - характеристики. Используется только три бита.
#define nRF_EN_DPL		2							//Динамическое изменение длины пакетов
#define nRF_EN_ACK_PAY	1							//Разрешить добавление в ответ ACK пользовательских данных
#define nRF_EN_DYN_ACK	0							//Разрешить передачу без подтверждения

#endif /* NRF24L01P_REG_H_ */