/*
 * ��������� ������� � �������������.
 * v1.4
 */ 

#include "CalcClock.h"


//********************** �������� ���� ������
const char mon[] PROGMEM = "��";
const char tus[] PROGMEM = "��";
const char wed[] PROGMEM = "��";
const char thr[] PROGMEM = "��";
const char fri[] PROGMEM = "��";
const char sat[] PROGMEM = "��";
const char sun[] PROGMEM = "��";
PGM_P const week_name_short[7] PROGMEM = {mon, tus, wed, thr, fri, sat, sun};

const char Monday[]		PROGMEM = "�����������";
const char Tuesday[]	PROGMEM = "�������";
const char Wednesday[]	PROGMEM = "�����";
const char Thursday[]	PROGMEM = "�������";
const char Friday[]		PROGMEM = "�������";
const char Saturday[]	PROGMEM = "�������";
const char Sunday[]		PROGMEM = "�����������";
PGM_P const PROGMEM week_name_full[7] = {Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday};
	
//********************** �������� �������, � ����������� ������
const char January[]	PROGMEM = "������";
const char February[]	PROGMEM = "�������";
const char March[]		PROGMEM = "�����";
const char April[]		PROGMEM = "������";
const char May[]		PROGMEM = "���";
const char June[]		PROGMEM = "����";
const char July[]		PROGMEM = "����";
const char August[]		PROGMEM = "�������";
const char September[]	PROGMEM = "��������";
const char October[]	PROGMEM = "�������";
const char November[]	PROGMEM = "������";
const char December[]	PROGMEM = "�������";
PGM_P const month_name_full[12] PROGMEM = {January, February, March, April, May, June, July, August, September, October, November, December};


/************************************************************************/
/* ��������� ������������� ��� ��� ������ �� ����� ����� ��� �����      */
/* PlaceDigit = 0 - �� ����� �����, 1 - �� ����� �����			        */
/************************************************************************/
void PlaceDay(const u08 NumDay, const u08 PlaceDigit){
	if (PlaceDigit == PLACE_HOURS){
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))), DIGIT3);		//1-� ������
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))+1), DIGIT2);	//2-� ������
	}
	else{
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))), DIGIT1);		//1-� ������
		sputc(pgm_read_byte((PGM_P)pgm_read_word(&(week_name_short[NumDay-1]))+1), DIGIT0);	//2-� ������
	}
}

/************************************************************************/
/* ���������� � ������ ������ ����� �� ����-������                      */
/************************************************************************/
void AddPhrase(PGM_P Value){

	for (u08 i = 0; pgm_read_byte(&Value[i]); i++){					//����� ����� �� �����
		sputc(pgm_read_byte(&Value[i]), UNDEF_POS);					//������
		sputc(S_SPICA, UNDEF_POS);									//���������� ����� ��������
	}
}

/************************************************************************/
/* �������� �������� ������ � ������ ������                             */
/************************************************************************/
void AddNameMonth(const u08 Month){
	u08 Val = BCDtoInt(Month);

	if (Val>12) return;
	AddPhrase((PGM_P)pgm_read_word(									//������ ��������� (������) ������ �� ���� ������
		&(month_name_full[Val-1]))
		);
}


/************************************************************************/
/* �������� �������� ��� ������ � ������ ������                         */
/************************************************************************/
void AddNameWeekFull(const u08 NumDay){
	if ((NumDay>7) || (NumDay<1)) return;
	AddPhrase((PGM_P)pgm_read_word(&(week_name_full[NumDay-1])));
}

/************************************************************************/
/* �������������� ��������� � BCD                                       */
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
/* ���� ������ �� ����.													*/
/* ���������� 1-��, 2-�� � �.�.	7-��									*/
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
/*  ������������� ��� � ����� �����                                     */
/************************************************************************/
u08 HourToInt(const u08 Hour){
	if ModeIs12(Watch.Mode)								//� ����������� �� �������� ������ ������ ����� �������������� ���� � 12-� ������� ������ ���� � 24-� ������� ������
		return ClockIsPM(Hour) ? BCDtoInt(Hour)+12:BCDtoInt(Hour);//12-� ������� �����
	else
		return BCDtoInt(Hour);							//24-� �������
}

/************************************************************************/
/* ������� �� 12-� ������� (����������) ������� � 24-� �������			*/
/*    (�����������).                                                    */
/************************************************************************/
inline u08 From12To24(volatile struct sClockValue *Clock){
	u08 i = (*Clock).Hour;								//����� ��� ����� �������
	
	if ModeIs12((*Clock).Mode){							//���� ���������������
		SetMode24((*Clock).Mode);
		if (i == 0x12)									//���� ������� ��� �������
			return ClockIsAM((*Clock).Mode)?0:0x12;		
		else{
			if ClockIsPM((*Clock).Mode){				//����� ������� ���� ��������� 12 �����
				i = AddDecBCD(i);
				i = AddOneBCD(i);
				return AddOneBCD(i);
			}
		}
	}
	return i;
}

/************************************************************************/
/* ������������ ���� � ����� �� ���������� ��������� ������ 			*/
/* � 01.01.1900. ���������� ������ ������ ���� ��� ��� �� 01.01.2013	*/
/* HourOffset - �������� � ����� ��� ��������� ������� ����	�� -12 �� 12*/
/* ���������� 1 ���� ��� ��������� � 0 ���� ������ ��� �������			*/
/************************************************************************/
u08 SecundToDateTime(u32 Secunds, volatile struct sClockValue *Value, s08 HourOffset){
	u32 restTime, restDay, Tmp = DAYS_01_01_2013;
	
	#define DAYS_IN_MONTH	(BCDtoInt(LastDayMonth((*Value).Month, (*Value).Year)))
	#define DAYS_IN_YEAR	(IsLeapYear((int)(BCDtoInt((*Value).Year)+2000))?366:365)
	
	if ((Secunds > (DAYS_01_01_2013*SEC_IN_DAY)) 
		&& 
		(HourOffset >=-12) && (HourOffset <=12)){
		//������ �������� �������
		if (HourOffset<0){
			HourOffset = (~HourOffset)+1;
			Secunds -= (u32)(HourOffset * SEC_IN_HOUR);
		}
		else
			Secunds += (u32)(HourOffset * SEC_IN_HOUR);
		restDay = Secunds/SEC_IN_DAY;						//���������� ���� � ���������
		//������ �������
		restTime = Secunds-(restDay*SEC_IN_DAY);			//���������� ������ ���������� ����� ��������� ����, �.�. ����� ���.
		(*Value).Hour = restTime/SEC_IN_HOUR;				//�����
		restTime -= (((u32)(*Value).Hour)*SEC_IN_HOUR);		//������� �����-������
		(*Value).Minute = restTime/SEC_IN_MIN;				//�����
		(*Value).Second = restTime-(((u32)(*Value).Minute)*SEC_IN_MIN);	//������
		(*Value).Hour = bin2bcd_u32((*Value).Hour, 1);		//�������� � BCD ����
		(*Value).Minute = bin2bcd_u32((*Value).Minute, 1);
		(*Value).Second = bin2bcd_u32((*Value).Second, 1);
		//������ ����
		(*Value).Year = YEAR_LIMIT;							//��������� ��� (2013)
		while(restDay > (Tmp+DAYS_IN_YEAR)){				//������������ ���������� ����� ��� � ��������� (� ������ ����������)
			Tmp += DAYS_IN_YEAR;
			(*Value).Year = AddOneBCD((*Value).Year);
		}
		restDay -= Tmp;										//������� ���� � ����
		(*Value).Month=1;
		Tmp = 0;
		while(restDay > (Tmp+DAYS_IN_MONTH)){				//������������ ���������� ����� ������� � ��������� ���������� ����� ��������� ���
			Tmp += DAYS_IN_MONTH;
			(*Value).Month = AddOneBCD((*Value).Month);
		}
		(*Value).Date = bin2bcd_u32(restDay - Tmp, 1);		//������� ���� � ������ ��� ���� ������. ����� ���������� � BCD ����
		return 1;
	}
	return 0;
}

/************************************************************************/
/* ���������� ���������� � ������� BCD, � ������ ����������� ��������   */
/* ���������� ���� � ������� RTC ������� ��� ������ � ���������������   */
/* �������																*/
/************************************************************************/
u08 AddClock(volatile struct sClockValue *Clck, enum tSetStatus _SetStatus){
	u08 i;

	switch (_SetStatus){
		case ssNone:									//���������� �����, ������� ��������
			break;
		case ssSecond:									//��������� ������ -  ���������� ������� � ����
			return 0;
			break;
		case ssMinute:									//��������� �����
			return ((*Clck).Minute == 0x59)? 0 : AddOneBCD((*Clck).Minute);
			break;
		case ssHour:									//�����
			i = From12To24(Clck);						//������������� 12-� ������� ���� � 24-� �������
			if (i == 0x23)								//��������� �������
				return 0;
			else
				return AddOneBCD(i);
			break;
		case ssDate:									//����. ���� ���������� ������ � ������� �.�. ���� ����������� ���������� ����. � ��� �� ������������� � ���������� ����� ���
			if (ClockStatus != csSet)
				return ((*Clck).Date == 0x31)? 0x01 : AddOneBCD((*Clck).Date);//� ����� ����������� ����� ���� �� ��������� � ���������, ������� ���������� ���� �� ����������.
			else
				return ((*Clck).Date == LastDayMonth((*Clck).Month, (*Clck).Year))? 0x1 : AddOneBCD((*Clck).Date);
			break;
		case ssMonth:									//�����. ����������� ���������� ���� � ���������� ������, � ���� �� ������ ��� � ���������� ������ �� ����� ����� ������������.
			i = ((*Clck).Month == 0x12)? 1 : AddOneBCD((*Clck).Month);
			while((*Clck).Date > LastDayMonth(i, (*Clck).Year))
				i = (i == 0x12)? 1 : AddOneBCD(i);
			return i;
			break;
		case ssYear:									//���. ����������� ���������� ���� � ��������� ����, ���� ���� ������ �� ������� ��������� ���
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
/* ���������� ���������� � ������� BCD, � ������ ����������� ��������   */
/* ���������� ���� � ������� RTC ������� ��� ������ � ���������������   */
/* �������																*/
/************************************************************************/
u08 DecClock(volatile struct sClockValue *Clck, enum tSetStatus _SetStatus){

	u08 i;
	
	switch (_SetStatus){
		case ssNone:									//���������� �����, ������� ��������
			break;
		case ssSecond:									//��������� ������ - ���������� ������� � ����
			return 0;
			break;
		case ssMinute:									//��������� �����
			return ((*Clck).Minute == 0x0)? 0x59 : DecOneBCD((*Clck).Minute);
			break;
		case ssHour:									//�����
			i = From12To24(Clck);						//������������� 12-� ������� ���� � 24-� �������
			if (i)										//�������
				return DecOneBCD(i);
			else
				return 0x23;
			break;
		case ssDate:									//����. ���� ���������� ������ � ������� �.�. ���� ����������� ���������� ����. � ��� �� ������������� � ���������� ����� ���
			if (ClockStatus != csSet)					//��� ����������� ���� ������ �� 31
				return ((*Clck).Date == 0x01)? 0x31: DecOneBCD((*Clck).Date); //��. ����������� � ����������� ����� � ������� AddClock
			else
				return ((*Clck).Date == 1)? LastDayMonth((*Clck).Month, (*Clck).Year) : DecOneBCD((*Clck).Date);
			break;
		case ssMonth:									//�����. ����������� ���������� ���� � ���������� ������, � ���� �� ������ ��� � ���������� ������ �� ����� ����� ������������.
			i = ((*Clck).Month == 0x1)? 0x12 : DecOneBCD((*Clck).Month);
			while((*Clck).Date > LastDayMonth(i, (*Clck).Year))
				i = (i == 0x1)? 0x12 : DecOneBCD(i);
			return i;
			break;
		case ssYear:									//���. ����������� ���������� ���� � ���������� ����. ���� ���������� ��������� ������ �� ��� ������������.
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

