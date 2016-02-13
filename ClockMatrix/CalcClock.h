/*
 * CalcClock.h
 * Различные вычисления в формате BCD и вычисления дат и дней недели
 * Created: 10.05.2013 14:54:51
 *  Author: 
 */ 

#ifndef CALCCLOCK_H_
#define CALCCLOCK_H_

#include <avr/pgmspace.h>
#include "avrlibtypes.h"
#include "Clock.h"
#include "Display.h"
#include "i2c.h"

//Макросы вычислений. Все BCD в виде байта! За исключением Yea в IsLeapYear(Yea)
#define BCDtoInt(bcd)	((((bcd & 0xf0) >> 4)*10) + (bcd & 0xf))				//BCD в Int без знака
#define AddDecBCD(Mc)	(((Mc & 0xf0)+0x10) & 0xf0)								//Увеличение десятков в формате BCD
#define AddOneBCD(bcd)	(((bcd & 0x0f) == 9)? AddDecBCD(bcd) : (bcd+1))			//Увеличить bcd на 1.
#define DecDecBCD(Mc)	(((Mc & 0xf0) == 0)?Mc : (((Mc & 0xf0)-0x10) | 0x9))	//уменьшение десятков в формате BCD
#define DecOneBCD(bcd)	(((bcd & 0x0f) == 0)? DecDecBCD(bcd) : (bcd-1))			//Уменьшить bcd на 1.
#define IsLeapYear(Yea) ((Yea % 4) == 0)										//Високосный год. Кратность 100 и 400 не проверяется поскольку часы работают только в 21 веке
#define LastDayFeb(Ye)	(IsLeapYear((int)(BCDtoInt(Ye)+2000))? 0x29 : 0x28)		//Количество дней в феврале заданного года
#define LastDayMonth(Mo, Ye) ((Mo == 0x2)? LastDayFeb(Ye) : (((Mo == 0x4) || (Mo == 0x6) || (Mo == 0x9) || (Mo == 0x11))?0x30:0x31))  //Последний день месяца
#define WeekInternal(wc) ((wc == 1)?7:(wc-1))									//Пересчет номера дня недели в формат функции what_day, (1-пн, 2-вт, и.т.д.) из формата RTC (1-вс,2-пн и т.д.)
#define WeekRTC(wc)		((wc == 7)?1:(wc+1))									//Обратная операция			

#define Tens(bcd)		((bcd >> 4) & 0xf)										//Взять цифру десятков в bcd в виде числа
#define	Unit(bcd)		(bcd & 0xf)												//Взять цифру едениц

#define SEC_IN_MIN		60														//Секунд в минуте
#define SEC_IN_HOUR		3600													//Секунд в часе
#define YEAR_LIMIT		0x13													//Год ограничивающий границу рассчетов дат в переводе секунд в dateime, в формате BCD
#define DAYS_01_01_2013	41272UL													//Точное количество дней прошедших с 01.01.1900 по 01.01.2013
#define SEC_IN_DAY		86400UL													//Секунд в сутках

u08 what_day(const u08 date, const u08 month, const u08 year);					//День недели по дате. Возвращает 1-пн, 2-вт и т.д. 7-вс
u08 AddClock(volatile struct sClockValue *Clck, 								//Рассчитывается следующее значение счетчика в соответствии с типом (например для минут после 59 идет 00)
					enum tSetStatus _SetStatus);
u08 DecClock(volatile struct sClockValue *Clck,									//Рассчитывается предыдущее значение счетчика
					enum tSetStatus _SetStatus);
					
void AddNameMonth(u08 Month);													//Добавить название месяца в строку вывода
void AddNameWeekFull(u08 NumDay);												//Добавить название недели
#define PLACE_HOURS		0														//Разместить вывод на знакоместа часов
#define PLACE_MINUT		1														//Разместить вывод на знакоместа минут
void PlaceDay(const u08 NumDay, const u08 PlaceDigit);							//Поместить двухбуквенное название дни недели

u32 bin2bcd_u32(u32 data, u08 result_bytes);									//Конвертер двоичного числа в BCD
u08 HourToInt(const u08 Hour);													//Конвертер числа часов в целое
u08 SecundToDateTime(u32 Secunds, volatile struct sClockValue *Value, s08 HourOffset);	//Конвертор числа секунд прошедших с 1.1.1900 в дату и время с учетом часового пояса

#endif /* CALCCLOCK_H_ */