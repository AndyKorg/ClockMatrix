/*
 * Различные расчеты и перекодировки.
 * v1.4
 */ 

#include "CalcClock.h"


//********************** Название дней недели
const char mon[] PROGMEM = "пн";
const char tus[] PROGMEM = "вт";
const char wed[] PROGMEM = "ср";
const char thr[] PROGMEM = "чт";
const char fri[] PROGMEM = "пт";
const char sat[] PROGMEM = "сб";
const char sun[] PROGMEM = "вс";
PGM_P const week_name_short[7] PROGMEM = {mon, tus, wed, thr, fri, sat, sun};

const char Monday[]		PROGMEM = "понедельник";
const char Tuesday[]	PROGMEM = "вторник";
const char Wednesday[]	PROGMEM = "среда";
const char Thursday[]	PROGMEM = "четверг";
const char Friday[]		PROGMEM = "пятница";
const char Saturday[]	PROGMEM = "суббота";
const char Sunday[]		PROGMEM = "воскресенье";
PGM_P const PROGMEM week_name_full[7] = {Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday};
	
//********************** Название месяцев, в родительном падеже
const char January[]	PROGMEM = "января";
const char February[]	PROGMEM = "февраля";
const char March[]		PROGMEM = "марта";
const char April[]		PROGMEM = "апреля";
const char May[]		PROGMEM = "мая";
const char June[]		PROGMEM = "июня";
const char July[]		PROGMEM = "июля";
const char August[]		PROGMEM = "августа";
const char September[]	PROGMEM = "сентября";
const char October[]	PROGMEM = "октября";
const char November[]	PROGMEM = "ноября";
const char December[]	PROGMEM = "декабря";
PGM_P const month_name_full[12] PROGMEM = {January, February, March, April, May, June, July, August, September, October, November, December};


/************************************************************************/
/* Поместить двухбуквенное имя дня недели на место часов или минут      */
/* PlaceDigit = 0 - на место часов, 1 - на место минут			        */
/************************************************************************/
void PlaceDay(const u08 NumDay, const u08 PlaceDigit){
	if (PlaceDigit == PLACE_HOURS){
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))), DIGIT3);		//1-й символ
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))+1), DIGIT2);	//2-й символ
	}
	else{
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))), DIGIT1);		//1-й символ
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))+1), DIGIT0);	//2-й символ
	}
}

/************************************************************************/
/* Добавление в строку вывода фразы из флаш-паямти                      */
/************************************************************************/
void AddPhrase(PGM_P Value){

	for (u08 i = 0; pgm_read_byte(&Value[i]); i++){					//Вывод фразы на экран
		sputc(pgm_read_byte(&Value[i]), UNDEF_POS);					//Символ
		sputc(S_SPICA, UNDEF_POS);									//промежуток между символам
	}
}

/************************************************************************/
/* Добавить название месяца в строку вывода                             */
/************************************************************************/
void AddNameMonth(const u08 Month){
	u08 Val = BCDtoInt(Month);

	if (Val>12) return;
	AddPhrase((PGM_P)pgm_read_word(									//Чтение указателя (адреса) строки во флаш памяти
		&(month_name_full[Val-1]))
		);
}


/************************************************************************/
/* Добавить название дня недели в строку вывода                         */
/************************************************************************/
void AddNameWeekFull(const u08 NumDay){
	if ((NumDay>7) || (NumDay<1)) return;
	AddPhrase((PGM_P)pgm_read_word(&(week_name_full[NumDay-1])));
}

/************************************************************************/
/* Преобразование двоичного в BCD                                       */
/************************************************************************/
u32 bin2bcd_u32(u32 data, u08 result_bytes){
	u32 result = 0; /*result*/
	for (u08 cnt_bytes=(4 - result_bytes); cnt_bytes; cnt_bytes--) /* adjust input bytes */
	data <<= 8;
	for (u08 cnt_bits=(result_bytes << 3); cnt_bits; cnt_bits--) { /* bits shift loop */
		/*result BCD nibbles correction*/
		result += 0x33333333;
		/*result correction loop*/
		for (u08 cnt_bytes=4; cnt_bytes; cnt_bytes--) {
			u08 corr_byte = result >> 24;
			if (!(corr_byte & 0x08)) corr_byte -= 0x03;
			if (!(corr_byte & 0x80)) corr_byte -= 0x30;
			result <<= 8; /*shift result*/
			result += corr_byte; /*set 8 bits of result*/
		}
		/*shift next bit of input to result*/
		result <<= 1;
		if (((u08)(data >> 24)) & 0x80)
		result |= 1;
		data <<= 1;
	}
	return(result);
}


/************************************************************************/
/* День недели по дате.													*/
/* Возвращает 1-пн, 2-вт и т.д.	7-вс									*/
/************************************************************************/
u08 what_day(const u08 date, const u08 month, const u08 year){
	int d = (int)BCDtoInt(date), 
		m = (int)BCDtoInt(month),
		y = (int)(2000+BCDtoInt(year));
	if ((month == 1)||(month==2)){
		y--;
		d += 3;
	}
	return (1 + (d + y + y/4 - y/100 + y/400 + (31*m+10)/12) % 7);
}

/************************************************************************/
/*  Преобразовать час в целое число                                     */
/************************************************************************/
u08 HourToInt(const u08 Hour){
	if ModeIs12(Watch.Mode)								//В зависимости от текущего режима работы часов рассчитывается либо в 12-и часовом режиме либо в 24-х часовом режиме
		return ClockIsPM(Hour) ? BCDtoInt(Hour)+12:BCDtoInt(Hour);//12-и часовой режим
	else
		return BCDtoInt(Hour);							//24-х часовой
}

/************************************************************************/
/* Перевод из 12-и часовой (английской) системы в 24-х часовую			*/
/*    (французскую).                                                    */
/************************************************************************/
inline u08 From12To24(volatile struct sClockValue *Clock){
	u08 i = (*Clock).Hour;								//Вроде так будет быстрее
	
	if ModeIs12((*Clock).Mode){							//Надо преобразовывать
		SetMode24((*Clock).Mode);
		if (i == 0x12)									//Если полдень или полночь
			return ClockIsAM((*Clock).Mode)?0:0x12;		
		else{
			if ClockIsPM((*Clock).Mode){				//После полудня надо прибавить 12 часов
				i = AddDecBCD(i);
				i = AddOneBCD(i);
				return AddOneBCD(i);
			}
		}
	}
	return i;
}

/************************************************************************/
/* Рассчитывает дату и время по количеству прошедших секунд 			*/
/* с 01.01.1900. Количество секунд должно быть для дат от 01.01.2013	*/
/* HourOffset - смещение в часах для требуемой часовой зоны	от -12 до 12*/
/* Возвращает 1 если все нормально и 0 если ошибка при расчете			*/
/************************************************************************/
u08 SecundToDateTime(u32 Secunds, volatile struct sClockValue *Value, s08 HourOffset){
	u32 restTime, restDay, Tmp = DAYS_01_01_2013;
	
	#define DAYS_IN_MONTH	(BCDtoInt(LastDayMonth((*Value).Month, (*Value).Year)))
	#define DAYS_IN_YEAR	(IsLeapYear((int)(BCDtoInt((*Value).Year)+2000))?366:365)
	
	if ((Secunds > (DAYS_01_01_2013*SEC_IN_DAY)) 
		&& 
		(HourOffset >=-12) && (HourOffset <=12)){
		//Расчет местного времени
		if (HourOffset<0){
			HourOffset = (~HourOffset)+1;
			Secunds -= (u32)(HourOffset * SEC_IN_HOUR);
		}
		else
			Secunds += (u32)(HourOffset * SEC_IN_HOUR);
		restDay = Secunds/SEC_IN_DAY;						//Количество дней в интервале
		//Расчет времени
		restTime = Secunds-(restDay*SEC_IN_DAY);			//Количество секунд оставшихся после вычитания дней, т.е. время дня.
		(*Value).Hour = restTime/SEC_IN_HOUR;				//Часов
		restTime -= (((u32)(*Value).Hour)*SEC_IN_HOUR);		//Остаток минут-секунд
		(*Value).Minute = restTime/SEC_IN_MIN;				//Минут
		(*Value).Second = restTime-(((u32)(*Value).Minute)*SEC_IN_MIN);	//Секунд
		(*Value).Hour = bin2bcd_u32((*Value).Hour, 1);		//Приводим к BCD виду
		(*Value).Minute = bin2bcd_u32((*Value).Minute, 1);
		(*Value).Second = bin2bcd_u32((*Value).Second, 1);
		//Расчет даты
		(*Value).Year = YEAR_LIMIT;							//Стартовый год (2013)
		while(restDay > (Tmp+DAYS_IN_YEAR)){				//Определяется количество целых лет в интервале (с учетом високосных)
			Tmp += DAYS_IN_YEAR;
			(*Value).Year = AddOneBCD((*Value).Year);
		}
		restDay -= Tmp;										//Остаток дней в году
		(*Value).Month=1;
		Tmp = 0;
		while(restDay > (Tmp+DAYS_IN_MONTH)){				//Определяется количество целых месяцев в интервале оставшемся после вычитания лет
			Tmp += DAYS_IN_MONTH;
			(*Value).Month = AddOneBCD((*Value).Month);
		}
		(*Value).Date = bin2bcd_u32(restDay - Tmp, 1);		//Остаток дней в месяце это день месяца. Сразу приводится к BCD виду
		return 1;
	}
	return 0;
}

/************************************************************************/
/* Увеличение переменной в формате BCD, с учетом ограничений отсчетов   */
/* возвращает байт в формате RTC готовый для записи в соответствующий   */
/* регистр																*/
/************************************************************************/
u08 AddClock(volatile struct sClockValue *Clck, enum tSetStatus _SetStatus){
	u08 i;

	switch (_SetStatus){
		case ssNone:									//Нормальный режим, никаких действий
			break;
		case ssSecond:									//Установка секунд -  сбрасывает секунды в ноль
			return 0;
			break;
		case ssMinute:									//Установка минут
			return ((*Clck).Minute == 0x59)? 0 : AddOneBCD((*Clck).Minute);
			break;
		case ssHour:									//часов
			i = From12To24(Clck);						//Преобразовать 12-и часовой день в 24-х часовой
			if (i == 0x23)								//Следующая полночь
				return 0;
			else
				return AddOneBCD(i);
			break;
		case ssDate:									//Даты. Надо записывать вместе с месяцем т.к. есть ограничения количества дней. А так же пересчитывать и записывать номер дня
			if (ClockStatus != csSet)
				return ((*Clck).Date == 0x31)? 0x01 : AddOneBCD((*Clck).Date);//В режим будильников номер года не участвует в установке, поэтому количество дней не органичено.
			else
				return ((*Clck).Date == LastDayMonth((*Clck).Month, (*Clck).Year))? 0x1 : AddOneBCD((*Clck).Date);
			break;
		case ssMonth:									//месяц. Учитывается количество дней в выбираемом месяце, и если их больше чем в выбираемом месяце то такой месяц пропускается.
			i = ((*Clck).Month == 0x12)? 1 : AddOneBCD((*Clck).Month);
			while((*Clck).Date > LastDayMonth(i, (*Clck).Year))
				i = (i == 0x12)? 1 : AddOneBCD(i);
			return i;
			break;
		case ssYear:									//год. Учитывается количество дней в выбранном году, если дней больше то берется следующий год
			i = ((*Clck).Year == 0x99)? 0 : AddOneBCD((*Clck).Year);
			while((*Clck).Date > LastDayMonth((*Clck).Month, i))
				i = (i == 0x99)? 0 : AddOneBCD(i);
			return i;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

/************************************************************************/
/* Уменьшение переменной в формате BCD, с учетом ограничений отсчетов   */
/* возвращает байт в формате RTC готовый для записи в соответствующий   */
/* регистр																*/
/************************************************************************/
u08 DecClock(volatile struct sClockValue *Clck, enum tSetStatus _SetStatus){

	u08 i;
	
	switch (_SetStatus){
		case ssNone:									//Нормальный режим, никаких действий
			break;
		case ssSecond:									//Установка секунд - сбрасывает секунды в ноль
			return 0;
			break;
		case ssMinute:									//Установка минут
			return ((*Clck).Minute == 0x0)? 0x59 : DecOneBCD((*Clck).Minute);
			break;
		case ssHour:									//часов
			i = From12To24(Clck);						//Преобразовать 12-и часовой день в 24-х часовой
			if (i)										//Полночь
				return DecOneBCD(i);
			else
				return 0x23;
			break;
		case ssDate:									//Даты. Надо записывать вместе с месяцем т.к. есть ограничения количества дней. А так же пересчитывать и записывать номер дня
			if (ClockStatus != csSet)					//Для будильников даты всегда до 31
				return ((*Clck).Date == 0x01)? 0x31: DecOneBCD((*Clck).Date); //см. комментарий к аналогичной ветке в функции AddClock
			else
				return ((*Clck).Date == 1)? LastDayMonth((*Clck).Month, (*Clck).Year) : DecOneBCD((*Clck).Date);
			break;
		case ssMonth:									//месяц. Учитывается количество дней в выбираемом месяце, и если их больше чем в выбираемом месяце то такой месяц пропускается.
			i = ((*Clck).Month == 0x1)? 0x12 : DecOneBCD((*Clck).Month);
			while((*Clck).Date > LastDayMonth(i, (*Clck).Year))
				i = (i == 0x1)? 0x12 : DecOneBCD(i);
			return i;
			break;
		case ssYear:									//год. Учитывается количество дней в выбираемом году. Если количество превышает нужное то год пропускается.
			i = ((*Clck).Year == 0)? 0x99 : DecOneBCD((*Clck).Year);
			while((*Clck).Date > LastDayMonth((*Clck).Month, i))
				i = (i == 0)? 0x99 : DecOneBCD(i);
			return i;
			break;
		default:
			return 0;
		break;
	}
	return 0;
}

