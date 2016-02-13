/*
		Автор DI HALT (http://easyelectronics.ru)
		Язык: C
		Опубликовано 23 ноября 2010 года в 22:20
		Конечный автомат, а также процедуры инициализации TWI интерфейса Mega AVR
*/

#include "IIC_ultimate.h"

void DoNothing(void);
 
volatile u08 i2c_Do;						// Переменная состояния передатчика IIC
u08 i2c_InBuff[i2c_MasterBytesRX];			// Буфер прием при работе как Slave
u08 i2c_OutBuff[i2c_MasterBytesTX];			// Буфер передачи при работе как Slave
u08 i2c_SlaveIndex;							// Индекс буфера Slave
 
 
u08 i2c_Buffer[i2c_MaxBuffer];				// Буфер для данных работы в режиме Master
u08 i2c_index;								// Индекс этого буфера
u08 i2c_ByteCount;							// Число байт передаваемых
 
u08 i2c_SlaveAddress;						// Адрес подчиненного
 
u08 i2c_PageAddress[i2c_MaxPageAddrLgth];	// Буфер адреса страниц (для режима с sawsarp)
u08 i2c_PageAddrIndex;						// Индекс буфера адреса страниц
u08 i2c_PageAddrCount;						// Число байт в адресе страницы для текущего Slave
 
											// Указатели выхода из автомата:
IIC_F MasterOutFunc = &DoNothing;			//  в Master режиме
IIC_F SlaveOutFunc 	= &DoNothing;			//  в режиме Slave
IIC_F ErrorOutFunc 	= &DoNothing;			//  в результате ошибки в режиме Master
 
 
#ifdef DEBUG_I2C
u08 	WorkLog[100];						// Лог пишем сюда
u08		WorkIndex=0;						// Индекс лога
#endif 
 
ISR(TWI_vect)								// Прерывание TWI Тут наше все.
{

/*
PORTB ^= 0x01;					// Дрыгаем ногой порта, для синхронизации логического анализатора и отметок вызова TWI
*/ 
 
#ifdef DEBUG_I2C
// Отладочный кусок. Вывод лога работы конечного автомата в буфер памяти, а потом. По окончании работы через UART на волю
if (WorkIndex <99)				// Если лог не переполнен
{
	WorkLog[WorkIndex]= TWSR;	// Пишем статус в лог
	WorkIndex++;
}

#endif

switch(TWSR & 0xF8)											// Отсекаем биты прескалера
	{
	case 0x00:	// Bus Fail (автобус сломался)
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
 
	case 0x08:	// Старт был, а затем мы:
			{
			if( (i2c_Do & i2c_type_msk)== i2c_sarp)			// В зависимости от режима
				{
				i2c_SlaveAddress |= 0x01;					// Шлем Addr+R
				}
			else											// Или 
				{
				i2c_SlaveAddress &= 0xFE;					// Шлем Addr+W
				}
 
			TWDR = i2c_SlaveAddress;				// Адрес слейва
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;  								// Go!
			break;
			}
 
	case 0x10:	// Повторный старт был, а затем мы
			{
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)		// В зависимости от режима
				{
				i2c_SlaveAddress |= 0x01;					// Шлем Addr+R
				}
			else
				{
				i2c_SlaveAddress &= 0xFE;					// Шлем Addr+W
				}
 
			// To Do: Добавить сюда обработку ошибок 
 
			TWDR = i2c_SlaveAddress;				// Адрес слейва
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;  	// Go!
			break;
			}
 
	case 0x18:	// Был послан SLA+W получили ACK, а затем:
			{
			if( (i2c_Do & i2c_type_msk) == i2c_sawp)		// В зависимости от режима
				{
				TWDR = i2c_Buffer[i2c_index];				// Шлем байт данных
				i2c_index++;							// Увеличиваем указатель буфера
 
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					i2c_i_am_slave<<TWEA|
					1<<TWEN|
					1<<TWIE;  // Go! 
 
				}
 
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)
				{
				TWDR = i2c_PageAddress[i2c_PageAddrIndex];	// Или шлем адрес странцы (по сути тоже байт данных)
				i2c_PageAddrIndex++;
															// Увеличиваем указатель буфера страницы
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					i2c_i_am_slave<<TWEA|
					1<<TWEN|
					1<<TWIE;	// Go!
				}
			}
			break;
 
	case 0x20:	// Был послан SLA+W получили NACK - слейв либо занят, либо его нет дома.
			{
			i2c_Do |= i2c_ERR_NA;	
															// Код ошибки
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;								// Шлем шине Stop
 
			MACRO_i2c_WhatDo_ErrorOut 						// Обрабатываем событие ошибки;
			break;
			}
 
	case 0x28: 	// Байт данных послали, получили ACK!  (если sawp - это был байт данных. если sawsarp - байт адреса страницы)
			{	// А дальше: 
			if( (i2c_Do & i2c_type_msk) == i2c_sawp)		// В зависимости от режима
				{
				if (i2c_index == i2c_ByteCount)				// Если был байт данных последний
					{																		
					TWCR = 	0<<TWSTA|
						1<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;						// Шлем Stop
 
					MACRO_i2c_WhatDo_MasterOut				// И выходим в обработку стопа
 
					}
				else
					{
					TWDR = i2c_Buffer[i2c_index];			// Либо шлем еще один байт
					i2c_index++;
					TWCR = 	0<<TWSTA|						
						0<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;  						// Go!
					}
				}
 
			if( (i2c_Do & i2c_type_msk) == i2c_sawsarp)		// В другом режиме мы
				{
				if(i2c_PageAddrIndex == i2c_PageAddrCount)	// Если последний байт адреса страницы
					{
					TWCR = 	1<<TWSTA|
						0<<TWSTO|
						1<<TWINT|
						i2c_i_am_slave<<TWEA|
						1<<TWEN|
						1<<TWIE;						// Запускаем Повторный старт!
					}
				else
					{												// Иначе 
					TWDR = i2c_PageAddress[i2c_PageAddrIndex];		// шлем еще один адрес страницы
					i2c_PageAddrIndex++;							// Увеличиваем индекс счетчика адреса страниц
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
 
	case 0x30:	//Байт ушел, но получили NACK причин две. 1я передача оборвана слейвом и так надо. 2я слейв сглючил.
			{
			i2c_Do |= i2c_ERR_NK;						// Запишем статус ошибки. Хотя это не факт, что ошибка. 
 
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// Шлем Stop
 
			MACRO_i2c_WhatDo_MasterOut					// Отрабатываем событие выхода
 
			break;
			}
 
	case 0x38:	//  Коллизия на шине. Нашелся кто то поглавней
			{
			i2c_Do |= i2c_ERR_LP;						// Ставим ошибку потери приоритета
 
			// Настраиваем индексы заново. 
			i2c_index = 0;
			i2c_PageAddrIndex = 0;
 
			TWCR = 	1<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// Как только шина будет свободна 
			break;										// попробуем передать снова.
			}
 
	case 0x40: // Послали SLA+R получили АСК. А теперь будем получать байты
			{
			if(i2c_index+1 == i2c_ByteCount)			// Если буфер кончится на этом байте, то 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Требуем байт, а в ответ потом пошлем NACK(Disconnect)
				}										// Что даст понять слейву, что мол хватит гнать. И он отпустит шину
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Или просто примем байт и скажем потом ACK
				}
 
			break;
			}
 
	case 0x48: // Послали SLA+R, но получили NACK. Видать slave занят или его нет дома. 
			{
			i2c_Do |= i2c_ERR_NA;
														// Код ошибки No Answer
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// Шлем Stop
 
			MACRO_i2c_WhatDo_ErrorOut					// Отрабатываем выходную ситуацию ошибки
			break;
			}
 
	case 0x50: // Приняли байт.
			{ 
			i2c_Buffer[i2c_index] = TWDR;				// Забрали его из буфера
			i2c_index++;
 
			// To Do: Добавить проверку переполнения буфера. А то мало ли что юзер затребует
 
			if (i2c_index+1 == i2c_ByteCount)			// Если остался еще один байт из тех, что мы хотели считать
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Затребываем его и потом пошлем NACK (Disconnect)
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Если нет, то затребываем следующий байт, а в ответ скажем АСК
				}
			break;
			}
 
	case 0x58:	// Вот мы взяли последний байт, сказали NACK слейв обиделся и отпал. 
			{
			i2c_Buffer[i2c_index] = TWDR;				// Взяли байт в буфер
			TWCR = 	0<<TWSTA|
				1<<TWSTO|
				1<<TWINT|
				i2c_i_am_slave<<TWEA|
				1<<TWEN|
				1<<TWIE;							// Передали Stop
 
			MACRO_i2c_WhatDo_MasterOut					// Отработали точку выхода
 
			break;
			}
 
// IIC  Slave ============================================================================
 
	case 0x68:	// RCV SLA+W Low Priority				// Словили свой адрес во время передачи мастером
	case 0x78:	// RCV SLA+W Low Priority (Broadcast)				// Или это был широковещательный пакет. Не важно
			{
			i2c_Do |= i2c_ERR_LP | i2c_Interrupted;		// Ставим флаг ошибки Low Priority, а также флаг того, что мастера прервали
 
			// Restore Trans after.
			i2c_index = 0;								// Подготовили прерваную передачу заново
			i2c_PageAddrIndex = 0;
			}											// И пошли дальше. Внимание!!! break тут нет, а значит идем в "case 60"
 
	case 0x60: // RCV SLA+W  Incoming?					// Или просто получили свой адрес
	case 0x70: // RCV SLA+W  Incoming? (Broascast)		// Или широковещательный пакет
			{
 
			i2c_Do |= i2c_Busy;							// Занимаем шину. Чтобы другие не совались
			i2c_SlaveIndex = 0;							// Указатель на начало буфера слейва, Неважно какой буфер. Не ошибемся
 
			if (i2c_MasterBytesRX == 1)					// Если нам суждено принять всего один байт, то готовимся принять  его
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Принять и сказать пошли все н... NACK!
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// А если душа шире чем один байт, то сожрем и потребуем еще ACK!
				}
			break;
			}
 
	case 0x80:	// RCV Data Byte						// И вот мы приняли этот байт. Наш или широковещательный. Не важно
	case 0x90:	// RCV Data Byte (Broadcast)
			{
			i2c_InBuff[i2c_SlaveIndex] = TWDR;			// Сжираем его в буфер.
 
			i2c_SlaveIndex++;							// Сдвигаем указатель
 
			if (i2c_SlaveIndex == i2c_MasterBytesRX-1) 	// Свободно место всего под один байт? 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Приянть его и сказать NACK!
				}
			else 
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Места еще дофига? Принять и ACK!
				}
			break;
			} 
 
	case 0x88: // RCV Last Byte							// Приянли последний байт
	case 0x98: // RCV Last Byte (Broadcast)
			{
			i2c_InBuff[i2c_SlaveIndex] = TWDR;			// Сожрали его в буфер
 
			if (i2c_Do & i2c_Interrupted)				// Если у нас был прерываный сеанс от имени мастера
				{
				TWCR = 	1<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Влепим в шину свой Start поскорей и сделаем еще одну попытку
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Если не было такого факта, то просто отвалимся и будем ждать
				}
 
			MACRO_i2c_WhatDo_SlaveOut					// И лениво отработаем наш выходной экшн для слейва
			break;
			}
 
 
	case 0xA0: // Ой, мы получили Повторный старт. Но чо нам с ним делать? 
			{
 
			// Можно, конечно, сделать вспомогательный автомат, чтобы обрабатывать еще и адреса внутренних страниц, подобно еепромке. 
			// Но я не стал заморачиваться. В этом случае делается это тут.
 
			TWCR = 	0<<TWSTA|
				0<<TWSTO|
				1<<TWINT|
				1<<TWEA|
				1<<TWEN|
				1<<TWIE;							// просто разадресуемся, проигнорировав этот посыл	
			break;
			}
 
 
 
	case 0xB0:  // Поймали свой адрес на чтение во время передачи Мастером			
			{
			i2c_Do |= i2c_ERR_LP | i2c_Interrupted;		// Ну чо, коды ошибки и флаг прерваной передачи.
 
 
			// Восстанавливаем индексы
			i2c_index = 0;
			i2c_PageAddrIndex = 0;
 
			}											// Break нет! Идем дальше						
 
	case 0xA8:	// // Либо просто словили свой адрес на чтение
			{
			i2c_SlaveIndex = 0;							// Индексы слейвовых массивов на 0
 
			TWDR = i2c_OutBuff[i2c_SlaveIndex];			// Чтож, отдадим байт из тех что есть.
 
			if(i2c_MasterBytesTX == 1)
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;						// Если он последний, мы еще на NACK в ответ надеемся 
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;						// А если нет, то  ACK ждем
				}
 
			break;
			} 
 
 
	case 0xB8: // Послали байт, получили ACK
			{
 
			i2c_SlaveIndex++;								// Значит продолжаем дискотеку. Берем следующий байт
			TWDR = i2c_OutBuff[i2c_SlaveIndex];				// Даем его мастеру
 
			if (i2c_SlaveIndex == i2c_MasterBytesTX-1)		// Если он последний был, то
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					0<<TWEA|
					1<<TWEN|
					1<<TWIE;							// Шлем его и ждем NACK
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;							// Если нет, то шлем и ждем ACK
				}
 
			break;
			}
 
	case 0xC0: // Мы выслали последний байт, больше у нас нет, получили NACK
	case 0xC8: // или ACK. В данном случае нам пох. Т.к. больше байтов у нас нет.
			{
			if (i2c_Do & i2c_Interrupted)			// Если там была прерваная передача мастера
				{									// То мы ему ее вернем
				i2c_Do &= i2c_NoInterrupted;
													// Снимем флаг прерваности
				TWCR = 	1<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;					// Сгенерим старт сразу же как получим шину.
				}
			else
				{
				TWCR = 	0<<TWSTA|
					0<<TWSTO|
					1<<TWINT|
					1<<TWEA|
					1<<TWEN|
					1<<TWIE;					// Если мы там одни, то просто отдадим шину
				}
 
			MACRO_i2c_WhatDo_SlaveOut				// И отработаем выход слейва. Впрочем, он тут
													// Не особо то нужен. Разве что как сигнал, что мастер
			break;									// Нас почтил своим визитом.
			}
 
	default:	break;
	}
}
 
void DoNothing(void)								// Функция пустышка, затыкать несуществующие ссылки
{
}
 
void Init_i2c(void)							// Настройка режима мастера
{
i2c_PORT |= 1<<i2c_SCL|1<<i2c_SDA;			// Включим подтяжку на ноги, вдруг юзер на резисторы пожмотился
i2c_DDR &= ~(1<<i2c_SCL|1<<i2c_SDA);
 
TWBR = 0x80;         						// Настроим битрейт
TWSR = 0x00;
}
 
void Init_Slave_i2c(IIC_F Addr)				// Настройка режима слейва (если нужно)
{
TWAR = i2c_MasterAddress;					// Внесем в регистр свой адрес, на который будем отзываться. 
											// 1 в нулевом бите означает, что мы отзываемся на широковещательные пакеты
SlaveOutFunc = Addr;						// Присвоим указателю выхода по слейву функцию выхода
 
TWCR = 	0<<TWSTA|
	0<<TWSTO|
	0<<TWINT|
	1<<TWEA|
	1<<TWEN|
	1<<TWIE;							// Включаем агрегат и начинаем слушать шину.
}