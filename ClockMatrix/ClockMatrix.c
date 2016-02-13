/************************************************************************/
/* ����	�� ��������� �����������                                        */
/* ��������� �� ���������� ����� � ����� Clock.h                        */
/* Andy Korg (c) 2014 �.												*/
/* http://radiokot.ru/circuit/digital/home/199/							*/
/************************************************************************/

#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <stddef.h>
#include <stdlib.h>

#include "IIC_ultimate.h"						//����������� ��������� I2C
#include "i2c.h"								//��������� ������ � ������������ �� I2C ����, � ��������� ����� ������ RTC
#include "avrlibtypes.h"
#include "bits_macros.h"
#include "EERTOSHAL.h"
#include "EERTOS.h"
#include "Clock.h"								//������ ����������� ��� �����
#include "CalcClock.h"							//������� ��� �����
#include "Display.h"							//����� � ����� �������
#include "keyboard.h"							//������������ ����������
#include "Alarm.h"								//����������� ������ �����������
#include "Sound.h"								//������ � SD-������ � ������
#include "eeprom.h"								//���������� �������� �����
#include "IRrecive.h"							//����� ������ �� ��-������ ����������
#include "sensors.h"							//��������� ����������� ��������� ��������
#include "ds18b20.h"							//������������ ������������ ds18b20
#include "nRF24L01P.h"							//�����������
#include "esp8266hal.h"							//������ ������ � WiFi
#ifdef VOLUME_IS_DIGIT
#include "Volume.h"
#endif

volatile struct sClockValue	Watch;				//������� �������� ����� � RTC � ��������� ����� ��������� ��������� � ���������� Mode
volatile enum tClockStatus ClockStatus;			//������� ����� �����
volatile enum tSetStatus SetStatus;				//������� �������� ���������
volatile struct sClockValue *CurrentCount;		//������������ �� ������� ������� - ���� ��� ����������
struct sAlarm *CurrentShowAlarm;				//������� ���������
u08 CurrentAlarmDayShow,						//������� ���� ������ ��� �������� ����������
	CurrentSensorShow,							//������� ������������� ������
	CurrentSensorAlphaNumAndIrCode;				//������� ����� �������������� ������� � ����� ������� � ������ � ������� ������ ����� �� ������ ��� �������� ���
volatile u08 CountWaitingEndSet;				//������ ���������� ������� � ������� �������� �� ���� ������� ������ ��� ������������ �� �� ������� � �� �������� ������. ������������ ��� �������� �� ������ ���������

VOID_PTR_VOID Refresh = NULL;					//��������� �� ������� ������� ����� ������� ��� ���������� �������� ������

const u08 PROGMEM								//����� ��������� � ������ �������
	setting_word[] = "���������\xa0�����",
	alarm_set_word[] =	"������������������",	//��������!- ������������ ��������� �����
	tune_set_word[] = "����\xa0���������",
	tune_HZspeed_word[] = "�������� ������� ������  ",
	tune_sound_word[] = "����\xa0���������",
	tune_font_word[] =	"�����\xa0�����",
	each_hour_word[] = "�������\xa0",
	key_beep_word[] = "����\xa0������\xa0",

	sens_set_word[] = "�������\xa0���������",
	sens_set_name_word[] = "���\xa0�������",
	sens_set_adr_word[] = "�����\xa0�������",

	sd_card_ok_word[] = "�����",
	sd_card_err_word[] = "������",
	sd_card_word[] = " �����",

	const_alrm_vol_word[] = "���������-���������� ���������",
	inc_alrm_vol_word[] = "��������� ����������� ��������� �� ",
	max_alrm_vol_word[] = "���������",
	lvl_alrm_vol_word[] = "������",
	inet_start_word[] = "����� ����� �� ���������",
	inet_loaded_word[] = "����� ���������",
	inet_tune_word[] = "���� ���������",
	ip_address[] = "ip ����� ����� ",

#ifndef IR_SAMSUNG_ONLY
	ir_set_word[] = "����� ��-���������",
	ir_set_code_name[][4] = {					//������� ���� � ���� ������� ������ ��������������� ������� ������ � ������� IRcmdList
		" ok",									//������� ���� ��
		"stp",									//������� ���� Step
		"inc",									//��������� ������� �������
		"dec",									//��������� ������� �������
		"ply",									//������� ������������� ������� ����������
		"end"									//����������
	},
#endif
	on_word[] =	"��������",
	off_word[] = "���������"
	;
	

/************************************************************************/
/* ��������� ����� � ������� ������ �������������� ����� ���            */
/* Start = 1 - ������� ����������� �������, 0 - ������ �������� �����	*/
/************************************************************************/
#define PHRAZE_START	1
#define PHRAZE_CONTINUE	0

void WordPut(const u08 *Value, u08 Start){
	u08 i;

	if (Start){
		ClearDisplay();
		CreepInfinteSet();
	}
	for (i=0; pgm_read_byte(&Value[i]) != 0;i++){
		sputc(pgm_read_byte(&Value[i]), UNDEF_POS);
		sputc(S_SPICA, UNDEF_POS);				//���������� ����������� �����
	}
}


/************************************************************************/
/* �������� ������� ������������� ������������                          */
/************************************************************************/
#define NO_FLASH	0							//��������� ������� ���� ���������
#define HOUR_FLASH	1							//������ ������������ �����
#define MINUT_FLASH	2							//������ ������������ �����
#define ALL_FLASH	3							//������ ����� ������������
#define UNDEF_FLASH	4							//�� �������� ����� �������

void FlashSet(u08 Value){
	switch (Value){
		case NO_FLASH:
			sputc(S_FLASH_OFF, DIGIT3);			//��������� �������
			sputc(S_FLASH_OFF, DIGIT2);
			sputc(S_FLASH_OFF, DIGIT1);
			sputc(S_FLASH_OFF, DIGIT0);
			break;
		case HOUR_FLASH:
			sputc(S_FLASH_ON, DIGIT3);			//������ ��������� �����
			sputc(S_FLASH_ON, DIGIT2);
			sputc(S_FLASH_OFF, DIGIT1);
			sputc(S_FLASH_OFF, DIGIT0);
			break;
		case MINUT_FLASH:
			sputc(S_FLASH_OFF, DIGIT3);			//������ ��������� �����
			sputc(S_FLASH_OFF, DIGIT2);
			sputc(S_FLASH_ON, DIGIT1);
			sputc(S_FLASH_ON, DIGIT0);
			break;
		case ALL_FLASH:
			sputc(S_FLASH_ON, DIGIT3);			//������ ����� ���������
			sputc(S_FLASH_ON, DIGIT2);
			sputc(S_FLASH_ON, DIGIT1);
			sputc(S_FLASH_ON, DIGIT0);
		default:
			break;
	}
}

/************************************************************************/
/* ����� �������� ����� "����"                                          */
/************************************************************************/
void PhrazeWaitShow(void){
	FlashSet(ALL_FLASH);
	sputc('�', DIGIT3);
	sputc('�', DIGIT2);
	sputc('�', DIGIT1);
	sputc('�', DIGIT0);
}

/************************************************************************/
/* ������� ��������� ������� � ������ ������������						*/
/************************************************************************/
void SensotTestShow(void){

	ClearDisplay();
	for(u08 i=0; i<SENSOR_MAX;i++){
		if SensorTestModeIsOn(SensorFromIdx(i)){
			u08 SensAdr = bin2bcd_u32(SensorFromIdx(i).Adr>>1, 1);
			u08 SensVal = bin2bcd_u32(SensorFromIdx(i).Value, 1);
			sputc(Tens(SensAdr), DIGIT3);
			sputc(Unit(SensAdr), DIGIT2);
			if (SensorBatareyIsLow(SensorFromIdx(i))){	//������ ����� �������
				sputc('�', DIGIT1);						//�������
				sputc('�', DIGIT0);						//���
			}
			else if (SensorIsNo(SensorFromIdx(i))){//��� ������ � ���������� �� ������
				sputc('�', DIGIT1);						//�������
				sputc('�', DIGIT0);						//���
			}
			else{										//��� ���������, ��������� ������� �������� �������
				sputc(Tens(SensVal), DIGIT1);
				sputc(Unit(SensVal), DIGIT0);
			}
			break;	
		}
	}
}

/************************************************************************/
/* �������� ��������� ������� ������ ���                                */
/************************************************************************/
void EachHourBeepShow(void){
	ClearDisplay();
	WordPut(each_hour_word, PHRAZE_START);
	if (EachHourSettingIsSet())
		WordPut(on_word, PHRAZE_CONTINUE);
	else
		WordPut(off_word, PHRAZE_CONTINUE);
}

/************************************************************************/
/* �������� ��������� ����� ������������ ������                         */
/************************************************************************/
void KeyBeepShow(void){
	ClearDisplay();
	WordPut(key_beep_word, PHRAZE_START);
	if (KeyBeepSettingIsSet())
		WordPut(on_word, PHRAZE_CONTINUE);
	else
		WordPut(off_word, PHRAZE_CONTINUE);
}

#ifdef VOLUME_IS_DIGIT
void VolumeShow(u08 Vol, u08 VolName1, u08 VolName0){
	u32 i = bin2bcd_u32(Vol, 1);
	
	sputc(VolName1, DIGIT3);
	sputc(VolName0, DIGIT2);
	sputc(Tens((u08)i), DIGIT1);
	sputc(Unit((u08)i), DIGIT0);
	FlashSet(MINUT_FLASH);
	SoundOn(SND_TEST);
}

/************************************************************************/
/* �������� ��������� ����� ������� �������								*/
/************************************************************************/
void VolumeBeepShow(void){
	VolumeShow(VolumeClock[vtButton].Volume, 'K', '�');
}

/************************************************************************/
/* �������� ��������� ����� ����������� �������							*/
/************************************************************************/
void VolumeEachHourShow(void){
	VolumeShow(VolumeClock[vtEachHour].Volume, '�', '�');
}

/************************************************************************/
/* �������� ��������� ����� ����������									*/
/************************************************************************/
void VolumeAlarmShow(void){
	VolumeShow(VolumeClock[vtAlarm].Volume, '�', '�');
}

/************************************************************************/
/* �������� ����� ������������� ��������� ����������                    */
/************************************************************************/
void VolumeAlarmIncShow(void){
	if (VolumeClock[vtAlarm].LevelVol == vlConst){				//����� ����������� ������ ���������
		WordPut(const_alrm_vol_word, PHRAZE_START);
	}
	else{
		WordPut(inc_alrm_vol_word, PHRAZE_START);				//����� ������������ ������
		if (VolumeClock[vtAlarm].LevelVol == vlIncreas)			//�� ������
			WordPut(lvl_alrm_vol_word, PHRAZE_CONTINUE);
		else
			WordPut(max_alrm_vol_word, PHRAZE_CONTINUE);		//�� ������������� ������
	}
}
#endif

/************************************************************************/
/* �������� ���� ������ � ����� ��������� - ���������� ����������       */
/*  � ���� ����.														*/
/************************************************************************/
void AlarmNumDayShow(void){
	PlaceDay(CurrentAlarmDayShow, PLACE_HOURS);
	sputc((AlrmDyIsOn(*CurrentShowAlarm, CurrentAlarmDayShow))?'+':'-', DIGIT1);
	sputc(BLANK_SPACE, DIGIT0);
}

/************************************************************************/
/* �������� ����� ����������                                            */
/************************************************************************/
void AlarmNumShow(void){
	sputc('�', DIGIT3);
	sputc(CurrentShowAlarm->Id, DIGIT2);
	if (LitlNumAlarm())
		sputc('�', DIGIT0);
	else
		sputc('�', DIGIT0);
}

/************************************************************************/
/* �������� ������������ ������� ����������                             */
/************************************************************************/
void AlarmDurationShow(void){
	u08 Du = bin2bcd_u32(AlarmDuration(*CurrentShowAlarm), 1);
	
	AlarmNumShow();
	sputc(Tens(Du), DIGIT1);
	sputc(Unit(Du), DIGIT0);
}

/************************************************************************/
/* �������� ����� ���������� � ������ ��������� ���-���� ����������     */
/************************************************************************/
void AlarmNumSetShow(void){
	AlarmNumShow();
	if AlarmIsOn(*CurrentShowAlarm)
		sputc('+', DIGIT1);
	else
		sputc('-', DIGIT1);
}

/************************************************************************/
/* �������� ���                                                         */
/************************************************************************/
void ShowYear(void){
	FlashSet(ALL_FLASH);
	sputc(Tens(CENTURY_DEF), DIGIT3);
	sputc(Unit(CENTURY_DEF), DIGIT2);
	sputc(Tens((*CurrentCount).Year), DIGIT1);
	sputc(Unit((*CurrentCount).Year), DIGIT0);
}

/************************************************************************/
/* ����� �������� �����������.											*/
/*  ���� � ������� ������ ���� � ������������� ����������				*/
/************************************************************************/
void TemperatureShow(u08 Value, u08 Fixed){
	u08 Place1 = UNDEF_POS, Place2 = UNDEF_POS, Place3 = UNDEF_POS, Place0 = UNDEF_POS;
	if (Fixed != UNDEF_POS){
		Place1 = DIGIT1;
		Place2 = DIGIT2;
		Place3 = DIGIT3;
		Place0 = DIGIT0;
	}
	if (Value & 0x80){									//���� ������������� �� � �������������� ����
		sputc('�', Place3);
		Value = ~Value;
	}
	else
		sputc('+', Place3);
	Value = bin2bcd_u32(Value & 0x7f, 1);
	sputc(Tens(Value), Place2);
	sputc(Unit(Value), Place1);
	sputc('�', Place0);										// ���� �������
}


/************************************************************************/
/* ����� �������� �������	                                           */
/************************************************************************/
void SensorCreepShow(char *Nam, u08 Value){
	
	sputc(' ', UNDEF_POS);
	for(u08 j=0; j < SENSOR_LEN_NAME; j++){		//��� �������
		if (Nam[j]){
			sputc(Nam[j], UNDEF_POS);
			sputc(S_SPICA, UNDEF_POS);
		}
		else
			break;
	}
	sputc(S_SPICA, UNDEF_POS);
	if (Value != TMPR_NO_SENS){					//����������� �������������
		TemperatureShow(Value, UNDEF_POS);
		sputc('C', UNDEF_POS);					// ������� C
	}
	else{
		sputc('-', UNDEF_POS);
		sputc('-', UNDEF_POS);
	}
	sputc(S_SPICA, UNDEF_POS);
}

/************************************************************************/
/* ���� � ���� ������ ������� ������� � ������� ����������� �������     */
/************************************************************************/
void ShowDate(void){
	
	u08 i;
	
	AddNameWeekFull(what_day(Watch.Date, Watch.Month, Watch.Year));	//��� ��� ������
	sputc(' ', UNDEF_POS);
	sputc(S_SPICA, UNDEF_POS);
	if (Tens(Watch.Date))
		sputc(Tens(Watch.Date), UNDEF_POS);			//���� ������
	sputc(S_SPICA, UNDEF_POS);
	sputc(Unit(Watch.Date), UNDEF_POS);
	sputc(' ', UNDEF_POS);
	AddNameMonth(Watch.Month);						//�������� ������
	for(i=0; i<SENSOR_MAX; i++){					//����� ����������� ��� ��������
		if (SensorIsShow(SensorFromIdx(i))){
			if (SensorFromIdx(i).SleepPeriod == 0)	//��������� �� ������� �� ���� �������� � ������� ����������� �������
				SensorCreepShow(SensorFromIdx(i).Name, TMPR_NO_SENS);
			else
				SensorCreepShow(SensorFromIdx(i).Name, (SensorIsNo(SensorFromIdx(i)))?TMPR_NO_SENS:SensorFromIdx(i).Value);
		}
	}
}

/************************************************************************/
/* �������� ���� ���� ���, ���� ���������� ��� ��� �� ����� ������      */
/* ������� ������ �� ���������� ������� ��������� � GetSecondFunc		*/
/************************************************************************/
void StartDateShow(void){
	if (CreepCount() == 1)
		GetSecondFunc();
	else{
		ClearDisplay();
		CreepOn(1);										//������� ������
		ShowDate();
	}
}

/************************************************************************/
/* ���� ��� ��������� - ����� ��� ������ � ����� ������                 */
/************************************************************************/
void ShowDateSet(void){
	sputc(Tens((*CurrentCount).Date), DIGIT3);	//����
	sputc(Unit((*CurrentCount).Date), DIGIT2);
	sputc(Tens((*CurrentCount).Month), DIGIT1);
	sputc(Unit((*CurrentCount).Month), DIGIT0);
}

/************************************************************************/
/* ����� ������� �� �������� �������� �����								*/
/************************************************************************/
void TimeOutput(void){
	if (((*CurrentCount).Hour & 0xf0) == 0)				//�� �������� �� �������� ����
		sputc(BLANK_SPACE, DIGIT3);
	else
		sputc(Tens((*CurrentCount).Hour), DIGIT3);
	sputc(Unit((*CurrentCount).Hour), DIGIT2);
	sputc(Tens((*CurrentCount).Minute), DIGIT1);
	sputc(Unit((*CurrentCount).Minute), DIGIT0);
}

/************************************************************************/
/* ����� �������� ������� � ���� �� ������ SECOND_DAY_SHOW              */
/************************************************************************/
void ShowTime(void){
	static u08 StartDateShow = 0;
	
	if (!Watch.Second)									//��������� �������� ������ ������ ���� ������ ������
		StartDateShow = 0;
	StartDateShow++;
	if ((StartDateShow == SECOND_DAY_SHOW) && CreepIsOff()) {//���� ��������� ������� ������ � ����� � ��� ��� �� ��������
		if (espInstalled()){							//���� ������ esp ����������, �� ������� ��������� ��� �� ������� ������������ ������
			u08 i=0;
			espUartTx(ClkRead(CLK_CUSTOM_TXT), &i, 1);//����� ��������� ������� ������ ������ �������� ������� ������������� ������. ��� ������� �������������� ����������� �������. ��.ISR(ESP_UARTRX_vect) � ����� esp8266hal.c
		}
		else{
			ClearDisplay();
			CreepOn(1);
			ShowDate();
		}
	}
	else{
		if CreepIsOn() return;							//��������� ��������� ��������� ������� ������
		VertSpeed = VERT_SPEED;							//���������� �������� ������������� ��������������
		if (ClockStatus == csAlarm)						//� ������ ������������ ���������� ������ ����� ������������
			FlashSet(ALL_FLASH);
		else
			FlashSet(NO_FLASH);
		TimeOutput();
		if (OneSecond){									//�������� ����� �����
			plot(11,7,1);
			plot(12,7,0);
		}
		else{
			plot(11,7,0);
			plot(12,7,1);
		}
	}
}

/************************************************************************/
/* ������� ���� �� ������� � ������ ������ ������                       */
/************************************************************************/
void FontShow(void){
	FlashSet(ALL_FLASH);
	TimeOutput();
}

/************************************************************************/
/* ����� �����-������		                                            */
/************************************************************************/
void ShowSecond(void){
	CreepOff();
	FlashSet(NO_FLASH);
	sputc(Tens(Watch.Minute), DIGIT3);
	sputc(Unit(Watch.Minute), DIGIT2);
	sputc(Tens(Watch.Second), DIGIT1);
	sputc(Unit(Watch.Second), DIGIT0);
	if (OneSecond){											//�������� ����� �����
		plot(11,7,1);
		plot(12,7,0);
	}
	else{
		plot(11,7,0);
		plot(12,7,1);
	}
}

/********************************** ������� **************************************/
/************************************************************************/
/* ����� �� ������� ������ ������� ��� ���������                        */
/************************************************************************/
void SensorsNumShow(void){
	sputc('�', DIGIT3);
	sputc(CurrentSensorShow, DIGIT2);
	if SensorIsShow(SensorFromIdx(CurrentSensorShow))
		sputc('+', DIGIT1);
	else
		sputc('-', DIGIT1);
}

/************************************************************************/
/* ����� ������ ������� �� ���� ��� �������������� �������              */
/************************************************************************/
void SensorsAdrShow(void){
	sputc('�', DIGIT3);
	sputc(CurrentSensorShow, DIGIT2);
	sputc('�', DIGIT1);
	sputc(SensorFromIdx(CurrentSensorShow).Adr>>1, DIGIT0);
}

/************************************************************************/
/* ����� �� ������� ����� ������� ��� ���������                         */
/************************************************************************/
void SensorsAlphaShow(void){
	sputc(CurrentSensorShow, DIGIT3);
	sputc(SensorFromIdx(CurrentSensorShow).Name[0], DIGIT2);
	sputc(SensorFromIdx(CurrentSensorShow).Name[1], DIGIT1);
	sputc(SensorFromIdx(CurrentSensorShow).Name[2], DIGIT0);
}

/************************************************************************/
/* ����� ���������� ������� ��� �����������                             */
/************************************************************************/
void SensSetNext(void){
	CurrentSensorShow++;
	if (CurrentSensorShow == SENSOR_MAX)
		CurrentSensorShow = 0;
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ����������-���������� �������� �������								*/
/************************************************************************/
void SensSwitch(void){
	if SensorIsShow(SensorFromIdx(CurrentSensorShow))
		SensorShowOff(SensorFromIdx(CurrentSensorShow));
	else
		SensorShowOn(SensorFromIdx(CurrentSensorShow));
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* �������� ��������� �������.											*/
/************************************************************************/
u08 SensTestON(void){
	return SensorIsShow(SensorFromIdx(CurrentSensorShow))?1:0;
}

/************************************************************************/
/* ������������ ������ ������� �� ����                                  */
/************************************************************************/
inline u08 SensAdrNotValid(u08 Value){
	for(u08 i=0;i<SENSOR_MAX;i++)
		if ((SensorFromIdx(i).Adr == Value) && (i != CurrentSensorShow))
			return 1;											//����� ����� ��� ���� � ������� ��������, ���������� ���
	return 0;													//������ ������ ��� ��� 	
}
void SensAdrSet(void){
	do{
		SensorFromIdx(CurrentSensorShow).Adr += 0b10;
		if (SensorFromIdx(CurrentSensorShow).Adr == (SENSOR_ADR_MASK+0b10))
			SensorFromIdx(CurrentSensorShow).Adr = 0;
	} while (SensAdrNotValid(SensorFromIdx(CurrentSensorShow).Adr));
	if (SensorFromIdx(CurrentSensorShow).Adr == SENSOR_DS18B20)		//��� ������� �� ���� 1-ware ����� ��������� ���������
		StartMeasureDS18();
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ������������ ����� � ������� ���������� ��� ������ ����� �������     */
/* ��������� ������ ������� ����� �� 0xc0 �� 0xdf � �����				*/
/************************************************************************/
void SensNameSet(void){
	u08 val = SensorFromIdx(CurrentSensorShow).Name[CurrentSensorAlphaNumAndIrCode]; //�������� � ���������� ������ ���� ��� �������������

	if (val == 0)												//����� ������, ��������� ������ ������
		val = S_SPACE;
	else if (val == S_SPACE)									//������, ��������� ����� 0
		val = 0x30;
	else if (val == 0x39)										//����� 9, ��������� �����
		val = S_DOT;											
	else if (val == S_DOT)										//���� �����, ����� �
		val = CYRILLIC_BEGIN_CAPITAL;
	else if (val == CYRILLIC_END_CAPITAL)						//����� �, ��������� ������
		val = S_SPACE;
	else														//������ ��������� ������
		val++;
	SensorFromIdx(CurrentSensorShow).Name[CurrentSensorAlphaNumAndIrCode] = val;
	if (Refresh != NULL)
		Refresh();												//�������� �����
}

/************************************************************************/
/* ������������� ������� �������, ��� ��������� ��������� ����������	*/
/* �����������, ���� ������� ��� ��������� ��������� "����"				*/
/************************************************************************/
void SensWait(void){
	//u08 Tempr = SensorFromIdx(CurrentSensorShow).Value;

	ClearDisplay();
	if SensorIsSet(SensorFromIdx(CurrentSensorShow)){			//���� �����-�� ����� (�� ������������ ����� ���� �����, �� ��� ������ ����� �� ��������)
		if (SensorFromIdx(CurrentSensorShow).Value != TMPR_NO_SENS){								//�����-�� �������� �����
			TemperatureShow(SensorFromIdx(CurrentSensorShow).Value, DIGIT0);
		}
		else{
			sputc('p',DIGIT3);									//���� ������ �� ������������, �� ��� ������ �� ������ �������
			sputc(' ',DIGIT2);
			sputc('e',DIGIT1);
			sputc('r',DIGIT0);
		}
	}
	else
		PhrazeWaitShow();
}

#ifndef IR_SAMSUNG_ONLY
/************************************************************************/
/* ������� ������ ���� �� ������ �� � ������ ������ ���� �������        */
/************************************************************************/
void IRRecivCode(u08 AdresIR, u32 CommandIR){
	if (CommandIR){
		if (AdresIR == REMOT_TV_ADR){							//��� �� ���  �������� ������ ��� ��� ����� ������� �������� � ����������� ��������� �� ����� �����
			IRcmdList[CurrentSensorAlphaNumAndIrCode] = CommandIR;
			TimeoutReturnToClock(TIMEOUT_RET_CLOCK_MODE_MIN);	//������� ������� �������� �������� � �������� �����
		}
		sputc(S_FLASH_ON, DIGIT0);
		if (Refresh != NULL)
			Refresh();
	}
	SetTimerTask(IRStartFromDelay, SCAN_PERIOD_KEY);			//��������� �� ��������� ����� ��������� ���������
}

/************************************************************************/
/* ����� ��������� ���� ������� �� ������                               */
/************************************************************************/
void IRCodeShow(void){

	u08 x = CellStart(DIGIT0);
	
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][0]), DIGIT3);
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][1]), DIGIT2);
	sputc(pgm_read_byte(&ir_set_code_name[CurrentSensorAlphaNumAndIrCode][2]), DIGIT1);
	if (IRcmdList[CurrentSensorAlphaNumAndIrCode]){				//���� ���. ��������� ��� "�������"
		for (u08 byt = 0; byt != IR_NUM_BYTS; x++, byt++)
			for(u08 y = 0; y != 8; y++)
				plot(x,y, BitIsSet(((u08)(IRcmdList[CurrentSensorAlphaNumAndIrCode] >> MULTIx8(byt)) & 0xff), y)?1:0);
	}
	else
		sputc(S_MINUS, DIGIT0);									//��� ����
}

/************************************************************************/
/* �������� ������� ���					                                */
/************************************************************************/
void IRCodeSave(void){
	if (Refresh != NULL)
		Refresh();
	if (IRcmdList[CurrentSensorAlphaNumAndIrCode] != 0)			//�������� ����� ������ ������������ ���
		FlashSet(NO_FLASH);
}

/************************************************************************/
/* ����� ���������� ���� ������� ��� ������ ��	                        */
/************************************************************************/
u08 IRNextCode(void){
	CurrentSensorAlphaNumAndIrCode++;
	if (CurrentSensorAlphaNumAndIrCode == IR_NUM_COMMAND)
		return 1;
	if (Refresh != NULL)
		Refresh();
	return 0;
}

#endif	//IR_SAMSUNG_ONLY

/************************************************************************/
/* ����� IP ������ � ������ station                                     */
/************************************************************************/
void IP_Show(void){
	WordPut(ip_address, PHRAZE_START);
	if (espStationIP != NULL){
		for (u08 i=0; *(espStationIP+i);i++){					//����� ������
			sputc(*(espStationIP+i), UNDEF_POS);
			sputc(S_SPICA, UNDEF_POS);							//���������� ����������� �����
		}
		free(espStationIP);
		espStationIP = NULL;
	}
	else{														//����� ��� �� ������������, �����������	
		espGetIPStation();
	}
}

/************************************************************************/
/* ������� ��������� ������� ������ ������� ������� � ������            */
/* ������������ ������ ����������� � �������� �������� � �������� ����� */
/************************************************************************/
void Check0SecondEachMinut(void){
	TimeoutReturnToClock(0);									//��������� ������������� �������� � �������� �����. ���������� ��� �� ��� �������� ���� ������, ��� �� ��� �� ��������� ������ ���������� ��� ���������
	AlarmCheck();												//��������� ������������� ������� ����������� � ��������� �� ��� �������������
}

/************************************************************************/
/* ��������� ����� ���� ���������� ��� ���������� ������ ����		    */
/************************************************************************/
void SettingProcess(SECOND_RTC Func, u08 Flash){
	ClearDisplay();												//�������� �����
	VertSpeed = VERT_SPEED_FAST;								//������� ����� �������
	Func();														//�������� �����������
	if (Flash != UNDEF_FLASH){
		FlashSet(NO_FLASH);										//��������� ������� ���������
		if (Flash != NO_FLASH)
			FlashSet(Flash);									//��������� ���������� ������
	}
	Refresh = Func;												//���������� ������� ��� ���������� ������
}

/************************************************************************/
/* ��������� �������� ������� � ����                                    */
/************************************************************************/
void TimeDateCountSet(enum tSetStatus Value){
	u08 Flash = MINUT_FLASH, ctrlCount = clkadrMINUTE;
	SECOND_RTC EachSecondFunc = TimeOutput;
	switch (Value){
		case ssHour:
			Flash = HOUR_FLASH,
			ctrlCount = clkadrHOUR;
			break;
		case ssDate:
			Flash = HOUR_FLASH;
			ctrlCount = clkadrDATA;
			EachSecondFunc = ShowDateSet;
			break;
		case ssMonth:
			Flash = MINUT_FLASH;
			ctrlCount = clkadrMONTH;
			EachSecondFunc = ShowDateSet;
			break;
		case ssYear:
			Flash = ALL_FLASH;
			ctrlCount = clkadrYEAR;
			EachSecondFunc = ShowYear;
		default:
			break;
	}
	SettingProcess(EachSecondFunc, Flash);
	SetWrtAdrRTC(ctrlCount);
	SetSecondFunc(EachSecondFunc);
}

/************************************************************************/
/* ��������� ������ ������ � ��������� �����. ��������������� ��� ��    */
/* ������ ������� ��������� � ������ ������ � RTC						*/
/************************************************************************/
void SetClockStatus(enum tClockStatus Val, enum tSetStatus SetVal){

	const u08 *txt;

	if (	
		((ClockStatus == csAlarmSet) && (Val != csAlarmSet) && (SetStatus != ssNone))	//��������� ��������� �����������
		||
		((ClockStatus == csSensorSet) && (Val != csSensorSet) && (SetStatus != ssNone))	//��������� ��������� ��������
		||
		((ClockStatus == csTune) && (Val != csTune) && (SetStatus != ssNone))			//��� �����. SetStatus != ssNone - ��� �� ����������� ����� ��������� �������� �� �������� ������ � eeprom
#ifndef IR_SAMSUNG_ONLY
		||
		((ClockStatus == csIRCodeSet) && (Val != csIRCodeSet) && (SetStatus == ssIRCode) && (SetVal != ssIRCode))	//���� ���������� ����� ������ �� csIRCodeSet;ssIRCode
#endif
		){
		EeprmStartWrite();								//�������� � eeprom
		if (ClockStatus == csAlarmSet)
			espUartTx(ClkWrite(CLK_ALARM), (u08 *) CurrentShowAlarm, sizeof(struct sAlarm));//�������� ������ esp �� ��������� ����������
		if (ClockStatus == csTune){						//�������� �������� � ��������� � ������
			espVolumeTx(vtButton);
			espVolumeTx(vtEachHour);
			espVolumeTx(vtAlarm);
		}
		if (ClockStatus == csSensorSet){				//�������� ������ �� ��������� ��������� �������
			espSendSensor(CurrentSensorShow);
		}

#ifndef IR_SAMSUNG_ONLY
		if (SetStatus == ssIRCode)						//��� ����� �� ������ ��������� ������ ��, ������� ���������� ������� ����� ������ �� �������
			IRReciveRdyOn();
#endif
		}

	ClockStatus = Val;
	SetStatus = SetVal;

	switch (Val){
		case csAlarm:									//--------- ��������� ��������. � ��������� ShowTime ���� �������������� �������� ������ ������ ������� - ������� ��� ��������
		case csClock:									//--------- ����� �������� �������
			CurrentCount = &Watch;						//������� ������� RTC
			if CreepIsOn()								//���� ���� ������� ������ �� �������� �������
				ClearDisplay();
			ShowTime();									//����� ��������� �����
			SetSecondFunc(ShowTime);					//��������� ���������� ������ �������
			Refresh = ShowTime;
			break;
		case csSecond:									//--------- ����� ������
			if CreepIsOn()								//���� ���� ������� ������ �� �������� �������
				ClearDisplay();
			VertSpeed = VERT_SPEED;
			ShowSecond();
			SetWrtAdrRTC(clkadrSEC);					//����� �������� ������ � RTC
			SetSecondFunc(ShowSecond);
			Refresh = ShowSecond;
			break;
		case csTempr:									//--------- ����� �����������
			break;
		case csSet:										//--------- ��������� �������� �������
			CurrentCount = &Watch;						//������� ������� RTC
			switch (SetVal){
				case ssNone:
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(setting_word, PHRAZE_START);
					break;
				case ssMinute:
				case ssHour:
				case ssDate:
				case ssMonth:
				case ssYear:
					TimeDateCountSet(SetVal);
					break;
				default:
					while(1);
				break;
			}
			break;
		case csAlarmSet:								//--------- ��������� ����������
			switch(SetVal){
				case ssNone:							//����� ��������� �� ����
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(alarm_set_word, PHRAZE_START);
					break;
				case ssNumAlarm:						//����� ������ ����������
					CurrentShowAlarm = FirstAlarm();	//��������� ����� ����������
					CurrentCount = &(CurrentShowAlarm->Clock);//������� ������� ��������������� � ������� SetNextShowAlarm ���������� �� MenuControl ��� ������� ������ ���
					CurrentAlarmDayShow = ALARM_START_DAY;//������� ���� ������ ��� ��������� ���������� - �����������
				case ssAlarmSw:							//����������� ��������� ���������� ��������-�������
					SettingProcess(AlarmNumSetShow, NO_FLASH);
					if (SetVal == ssNumAlarm)
						sputc(S_FLASH_ON, DIGIT2);
					else
						sputc(S_FLASH_ON, DIGIT1);
					break;
				case ssMinute:							//��������� ������� � ���� ����������
				case ssHour:
				case ssDate:
				case ssMonth:
				case ssYear:
					TimeDateCountSet(SetVal);
					SetSecondFunc(Idle);				//��������� ������������� ����������
					break;
				case ssAlarmDelay:						//��������� ������� �������� ����������
					SettingProcess(AlarmDurationShow, MINUT_FLASH);
					break;
				case ssAlarmTuesd:						//��������� ��� ������ ������������ ����������
				case ssAlarmWedn:
				case ssAlarmThur:
				case ssAlarmFrd:
				case ssAlarmSat:
				case ssAlarmSun:
					CurrentAlarmDayShow++;				//������������ ��� ������ �� ����������� ������������
				case ssAlarmMondy:						//��������� ������������ � �������� ��� ������������ ����������
					SettingProcess(AlarmNumDayShow, MINUT_FLASH);
					break;
				default:
					break;
			}
			break;
		case csTune:									//--------- ��������� �����
			switch (SetVal){			
				case ssNone:							//����� ����� � ���������
				case ssTuneSound:						//����� ����� � ��������� �����
					SetSecondFunc(Idle);				//��������� ���� ����������
					if (SetVal == ssNone)
						txt = tune_set_word;
					else 
						txt = tune_sound_word;
					WordPut(txt, PHRAZE_START);
					break;
				case ssEvryHour:						//������ ������ ���
					ClearDisplay();
					EachHourBeepShow();
					Refresh = EachHourBeepShow;
					break;
				case ssKeyBeep:							//������ �� ������� ������
					ClearDisplay();
					KeyBeepShow();
					Refresh = KeyBeepShow;
					break;
				case ssFontPreTune:						//��������������� ����� � ��������� ������ ����
					WordPut(tune_font_word, PHRAZE_START);
					Refresh = NULL;
					break;
				case ssFontTune:						//��������� ������ ����
					ClearDisplay();
					WordPut(tune_font_word, PHRAZE_START);
					SettingProcess(FontShow, ALL_FLASH);
					SetSecondFunc(FontShow);
					break;
				case ssSensorTest:						//������ ������ ���� �� ����� �� ������� ��������
					SetSecondFunc(Idle);				
					SensotTestShow();
					Refresh = SensotTestShow;
					break;
#ifdef VOLUME_IS_DIGIT
				case ssVolumeBeep:						//��������� ������� ������
					ClearDisplay();
					VolumeBeepShow();
					Refresh = VolumeBeepShow;
					break;
				case ssVolumeEachHour:					//��������� ����������� �������
					ClearDisplay();
					VolumeEachHourShow();
					Refresh = VolumeEachHourShow;
					break;
				case ssVolumeAlarm:						//��������� ����������
					ClearDisplay();
					VolumeAlarmShow();
					Refresh = VolumeAlarmShow;
					break;
				case ssVolumeTypeAlarm:					//��� ����������� ��������� ����������
					ClearDisplay();
					VolumeAlarmIncShow();
					Refresh = VolumeAlarmIncShow;
					break;
#endif
				case ssHZSpeedTune:						//��������� �������� �������������� ���������
				case ssHZSpeedTSet:						//��������� ��������
					SetSecondFunc(Idle);
					WordPut(tune_HZspeed_word, PHRAZE_START);
					if (SetVal == ssHZSpeedTSet)
						ShowDate();
					Refresh = NULL;
					break;					
				case ssTimeSet:							//������ ��������� ������� �� ���������
				case ssTimeLoaded:						//����� ��������
				case ssTuneNet:							//��������������� ����� ��� ��������� ����
					SetSecondFunc(Idle);
					if (SetVal == ssTimeSet)
						txt = inet_start_word;
					else if (SetVal == ssTimeLoaded)			
						txt = inet_loaded_word;
					else 
						txt = inet_tune_word;
					WordPut(txt, PHRAZE_START);
					Refresh = NULL;						
					break;
				case ssIP_Show:							//�������� IP �����
					SetSecondFunc(Idle);
					IP_Show();
					Refresh = NULL;
					break;
				default:
					break;
			}
			break;
		case csSensorSet:								//--------- ��������� ��������
			switch(SetVal){
				case ssNone:							//����� � ��������� ��������
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(sens_set_word, PHRAZE_START);
					break;
				case ssSensNext:						//����� �������
					CurrentSensorShow = 0;				//������ � 0 �������
				case ssSensSwitch:						//���������-���������� �������
					SettingProcess(SensorsNumShow, NO_FLASH);
					if (SetVal == ssSensNext)
						sputc(S_FLASH_ON, DIGIT2);
					else
						sputc(S_FLASH_ON, DIGIT1);
					break;
				case ssSensPreAdr:						//����� � ������ ������ �������
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(sens_set_adr_word, PHRAZE_START);
					break;
				case ssSensAdr:							//���������� ����� ��� �������� �������
					SettingProcess(SensorsAdrShow, NO_FLASH);
					sputc(S_FLASH_ON, DIGIT0);
					break;
				case ssSensPreName:						//��������� � ����� �����
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(sens_set_name_word, PHRAZE_START);
					sputc(' ', UNDEF_POS);
					sputc(CurrentSensorShow, UNDEF_POS);
					sputc(' ',UNDEF_POS);
					break;
				case ssSensName1:						//����� ������ ����� �����
					CurrentSensorAlphaNumAndIrCode = 0;
					SettingProcess(SensorsAlphaShow, NO_FLASH);
					sputc(S_FLASH_ON, DIGIT2);
					break;
				case ssSensName2:						//����� ������ ����� �����
					FlashSet(NO_FLASH);
					sputc(S_FLASH_ON, DIGIT1);
					CurrentSensorAlphaNumAndIrCode++;
					break;
				case ssSensName3:						//����� ������� ����� �����
					FlashSet(NO_FLASH);
					sputc(S_FLASH_ON, DIGIT0);
					CurrentSensorAlphaNumAndIrCode++;
					break;
				case ssSensWaite:						//�������� �������. 
					SetSecondFunc(Idle);
					SensorFromIdx(CurrentSensorShow).Value = TMPR_NO_SENS;		//������� ��� ����������� ���������������
					SensorNoInBus(SensorFromIdx(CurrentSensorShow));			//������� �� ���� ���
					SettingProcess(SensWait, NO_FLASH);
					if (SensorFromIdx(CurrentSensorShow).Adr == SENSOR_DS18B20) //�������� ������� ��������� ����� ������� ���������
						StartMeasureDS18();
					else if (SensorFromIdx(CurrentSensorShow).Adr == (EXTERN_TEMP_ADR & SENSOR_ADR_MASK)) //� ��� ������� �� ���� I2C
						i2c_ExtTmpr_Read();
					break;
				default:
					break;
			}
			break;
#ifndef IR_SAMSUNG_ONLY
		case csIRCodeSet:								//--------- ��������� ������ �� ������
			switch (SetVal){
				case ssNone:
					SetSecondFunc(Idle);				//��������� ���� ����������
					WordPut(ir_set_word, PHRAZE_START);
					CurrentSensorAlphaNumAndIrCode = 0;//������ � ������ ������� �����
					break;
				case ssIRCode:							//������� ������� ��� ������� ������ ��
					IRReciveReady = IRRecivCode;		//� ������ ������ ���� ������ ������� ��������� ���� ������ ������
					SettingProcess(IRCodeShow, UNDEF_FLASH);
					break;
				default:
					break;
			}
			break;
#endif
		case csSDCardDetect:							//---------- ���������� ����� � ����� micro-SD, ��������� ��������� �������������
			ClearDisplay();
			switch (SetVal){
				case ssSDCardNo:						//�� ������� ���������� �����
					WordPut(sd_card_err_word, PHRAZE_START);
					break;
				case ssSDCardOk:						//����� ������� ��������������
					WordPut(sd_card_ok_word, PHRAZE_START);
					break;
				default:
					break;
			}
			WordPut(sd_card_word, PHRAZE_CONTINUE);
			CreepOn(2);									//���������� ����� ������ 2 ����
			SetSecondFunc(ShowTime);					//�� ��������� ������� ������ ������� �����
			Refresh = ShowTime;
			break;
		case csInternetErr:
			SetSecondFunc(Idle);						//��������� ���� ����������
			ClearDisplay();
			CreepInfinteSet();
			Refresh = NULL;								//�������������� ���������
			break;
		default:
			CurrentCount = &Watch;						//���� ����� ������������ ������ ��� ���� ��� �� ���������� �� ������� ��������������
			CurrentShowAlarm = NULL;
			break;
	}
}

int main(void)
{
	InitRTOS();
	RunRTOS();

	wdt_enable(WDTO_1S);		
//	set_sleep_mode(SLEEP_MODE_PWR_DOWN);						//TODO:�� ��������, �.�. �� ������ �������� ������� ���������� �� ������ ������� �������, �������� � �� �����
	Init_i2c();													//i2C
	Init_i2cRTC();												//RTC. ���� ����� ������ ���� �������� ��� ����� ������ ����� ������ RunRTOS() ��������� � RTOS ������������ ���������� ������������ ����������� RTC
	
	EepromInit();												//������������� EEPROM ������ ���������� �� ������������� �����������
	DisplayInit();												//������������� ������� ����� ���������� � ����� ����� ����� ���������� ����������
	Set0SecundFunc(Check0SecondEachMinut);						//��������� ������� ���������� ������ 0-� ������� � ������
	SetClockStatus(csClock, ssNone);							//����� ����������� �����������
	AlarmIni();													//����������
	SensorIni();												//������������� �������� ������� ��������. ������ ���������� ����� ������������� eeprom

	IRReciverInit();											//����� ������ ��������� ��

	InitKeyboard();												//����������

	Init_i2cExtTmpr();											//������������� �������� ������� �� ���� I2C
	StartMeasureDS18();											//����� ��������� ����������� �� ������� ds18b20
	
	SoundIni();													//��������� ��� ����� � ������ � SD-������

	KeyBeepSettingSetOff();										//���� ������ ���������
	KeyBeepSettingSwitch();
	EachHourSettingSetOff();									//�������� �������
	EachHourSettingSwitch();

	SetSecondFunc(ShowTime);									//���������� ����� ������ �������

	EeprmReadTune();											//������������ ��������� ����� ���� ��� ����
	
	nRF_Init();													//������������� ������ nRF24L01+
	espInit();													//������������� ������ esp

	while(1){
		Disable_Interrupt;
		if (ClockStatus == csPowerOff){							//������� �������. ��������
			wdt_disable();
			DisableIntRTC;										//��� �� RTC �� ������
			Enable_Interrupt;									//��� �� ��������� ������� ������
			sleep_enable();
			sleep_cpu();										//�������
			sleep_disable();									//����������
			wdt_enable(WDTO_1S);
			EnableIntRTC;
		}
		Enable_Interrupt;
		wdt_reset();
		TaskManager();
	}
}
