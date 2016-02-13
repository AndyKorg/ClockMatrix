/************************************************************************/
/* ����� �����. �� ������ � ������������� ���� �� SD ��������			*/
/* ������������ �������:												*/
/* ������ 0 - ���������� ����������� ���								*/
/* ������ 1 - ���������� �������										*/
/* ver 1.4																*/
/************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "EERTOS.h"
#include "Clock.h"
#include "i2c.h"												//�������� �������� ������� ��� ��������
#include "Sound.h"
#ifdef VOLUME_IS_DIGIT
	#include "Volume.h"
#endif

#ifndef F_CPU
#warning "F_CPU not define"
#endif

//------- ��������������� �����
#define STOP_TMR_BITRT	ClearBit(TIMSK, OCIE1A)					//���� ������ ���������������
#define START_TMR_BITRT	SetBit(TIMSK, OCIE1A)					//����� ������ ��������������
#define SOUND_OFF		TCCR0 &= ~( Bit(COM01) | Bit(COM00) )	//��������� ���� - ������ �������� ������ OC0
#define SOUND_ON		TCCR0 |= ( Bit(COM01) | Bit(COM00) )	//�������� ����

//------- �������� ������� ������
#define BUF_EMPTY		1										//����� ����
#define BUF_FULL		0
#define BUF0_READ		1										//������ �� ������ 0
#define BUF1_READ		0										//������ �� ������ 1

//���� ��� ���������� ���������� � ����������, ��� �� ��������� volatile, �.�. �� ������ ������ ��� �� ���������� ������������ � ���������� � � �������� ������
unsigned int PosInBuf;											//������� ������� � ������
unsigned char reading0buf;										//���� ������ �� ������ 0, � ��������� ������ �� ������ 1
unsigned char read_buf0[BUF_LEN], read_buf1[BUF_LEN];			//���� ������ �������
volatile unsigned char	buf1_empty = BUF_EMPTY,					
						buf0_empty = BUF_EMPTY;					//��������� �������

FATFS fs;														//FatFs object
						
unsigned long int
	DataChunkSize,												//���������� ������ ��������� �����
	CurrentChunk;												//������� ������������� �����

u08 StrikingClockCount;											//���������� ������ ��� �����
u08 SoundFlag = 0;												//��������� ������� (�� ������, ������ ��� � �.�.)

//����� SoundFlag. ����� �������� ��������� �������� ������� �������� _WORK, ����� ��������� _SETING
//��������� �������� �������
#define EACH_HOUR_BEEP_SETING	0											//��� ����� (�������) ������ ���
#define EachHourSettingOn		SetBit(SoundFlag, EACH_HOUR_BEEP_SETING)
#define EachHourSettingOff		ClearBit(SoundFlag, EACH_HOUR_BEEP_SETING)
#define EachHourSettingIsOn		BitIsSet(SoundFlag, EACH_HOUR_BEEP_SETING)

#define KEY_BEEP_SETING			1											//���� �� �������
#define KeyBeepSettingOn		SetBit(SoundFlag, KEY_BEEP_SETING)
#define KeyBeepSettingOff		ClearBit(SoundFlag, KEY_BEEP_SETING)
#define	KeyBeepSettingIsOn		BitIsSet(SoundFlag, KEY_BEEP_SETING)

//������� ���������
#define EACH_HOUR_BEEP_WORK		3											//���� ������ ��� ������
#define EachHourBegin			SetBit(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourEnd				ClearBit(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourIsWork			BitIsSet(SoundFlag, EACH_HOUR_BEEP_WORK)
#define EachHourIsNoWork		BitIsClear(SoundFlag, EACH_HOUR_BEEP_WORK)

#define KEY_BEEP_WORK			4											//���� ������ ������
#define KeyBeepBegin			SetBit(SoundFlag, KEY_BEEP_WORK)
#define KeyBeepEnd				ClearBit(SoundFlag, KEY_BEEP_WORK)
#define KeyBeepIsWork			BitIsSet(SoundFlag, KEY_BEEP_WORK)

#define	ALARM_BEEP_WORK			5											//������ ���������� ������
#define AlarmBeepBegin			SetBit(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepEnd			ClearBit(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepIsWork			BitIsSet(SoundFlag, ALARM_BEEP_WORK)
#define AlarmBeepIsNoWork		BitIsClear(SoundFlag, ALARM_BEEP_WORK)

//------- ���� ��� ����� 
#define BEEP_BITRATE		22050								//������� ��� ����� ������
#define BEEP_READ_PERIOD	5									//������ ������ ������ � ����� ��� ����� ������
#define BEEP_KEY_DELAY		25									//������������ �������� ��� ������� ������
#define BEEP_ON_DELAY		500									//������������ ����������� ������ ����� ��� ����� ������ ��� ��� ����������
#define BEEP_OFF_DELAY		500									//������������ ������������ ������� ����� ��� ����� ������ ��� ��� ����������
//TODO: �������� ����� ����������, ���-�� ���� ���������� ��� ������ ���������� �� ��������������� � SD-card
#define BEEP_SIN_LEN		256									//���������� �������� � ������� ������ sin_table
const u08 PROGMEM sin_table[] =									//������� ������ ��� ����� ������. 256 ��������
{
	0x7F, 0x7C, 0x79, 0x76, 0x73, 0x6F, 0x6C, 0x69, 0x66, 0x63, 0x60, 0x5D, 0x5A, 0x57, 0x54, 0x51,
	0x4E, 0x4C, 0x49, 0x46, 0x43, 0x40, 0x3E, 0x3B, 0x38, 0x36, 0x33, 0x31, 0x2E, 0x2C, 0x2A, 0x27,
	0x25, 0x23, 0x21, 0x1F, 0x1D, 0x1B, 0x19, 0x17, 0x15, 0x14, 0x12, 0x10, 0x0F, 0x0E, 0x0C, 0x0B,
	0x0A, 0x09, 0x07, 0x06, 0x05, 0x05, 0x04, 0x03, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x09,
	0x0A, 0x0B, 0x0C, 0x0E, 0x0F, 0x10, 0x12, 0x14, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x21, 0x23,
	0x25, 0x27, 0x2A, 0x2C, 0x2E, 0x31, 0x33, 0x36, 0x38, 0x3B, 0x3E, 0x40, 0x43, 0x46, 0x49, 0x4C,
	0x4E, 0x51, 0x54, 0x57, 0x5A, 0x5D, 0x60, 0x63, 0x66, 0x69, 0x6C, 0x6F, 0x73, 0x76, 0x79, 0x7C,
	0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8F, 0x92, 0x95, 0x98, 0x9B, 0x9E, 0xA1, 0xA4, 0xA7, 0xAA, 0xAD,
	0xB0, 0xB2, 0xB5, 0xB8, 0xBB, 0xBE, 0xC0, 0xC3, 0xC6, 0xC8, 0xCB, 0xCD, 0xD0, 0xD2, 0xD4, 0xD7,
	0xD9, 0xDB, 0xDD, 0xDF, 0xE1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEA, 0xEC, 0xEE, 0xEF, 0xF0, 0xF2, 0xF3,
	0xF4, 0xF5, 0xF7, 0xF8, 0xF9, 0xF9, 0xFA, 0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE,
	0xFE, 0xFE, 0xFE, 0xFE, 0xFD, 0xFD, 0xFD, 0xFC, 0xFC, 0xFB, 0xFA, 0xF9, 0xF9, 0xF8, 0xF7, 0xF5,
	0xF4, 0xF3, 0xF2, 0xF0, 0xEF, 0xEE, 0xEC, 0xEA, 0xE9, 0xE7, 0xE5, 0xE3, 0xE1, 0xDF, 0xDD, 0xDB,
	0xD9, 0xD7, 0xD4, 0xD2, 0xD0, 0xCD, 0xCB, 0xC8, 0xC6, 0xC3, 0xC0, 0xBE, 0xBB, 0xB8, 0xB5, 0xB2,
	0xB0, 0xAD, 0xAA, 0xA7, 0xA4, 0xA1, 0x9E, 0x9B, 0x98, 0x95, 0x92, 0x8F, 0x8B, 0x88, 0x85, 0x82,
};
#if (BUF_LEN < BEEP_SIN_LEN)
	#error "Size tables of sines less than the buffer sound, reduce the size of tables of sines"
#endif
/************************************************************************/
/* ��������� ���������� ���												*/
/************************************************************************/
//------ ������������� ������� ��������
void timer1_init()
{
	TCCR1B =	(1 << WGM12) |									//����� CTC
				(1 << CS10);									//��� �������
	TCNT1 = 0;
	OCR1A = (F_CPU/BEEP_BITRATE);								//�������� �������� ��� ������������� �� �����, �.�. �� �������������� ��� ������ ��������������� �����
	STOP_TMR_BITRT;												//���� ������ ����������
}
//------- ������������� ������� ���������� ���
void pwm_init()
{
	TCCR0 =
			(1<<WGM01) | (1<<WGM00) |							//����� Fast PWM
			(0<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0 = 0;
	SOUND_OFF;
	OCR0 = 128;
	DDRB |= (1<<PORTB3);										//���� OC0 �� �����
}

//------- ��������� ������� �� ������. ���������� � ���� ��������� ���������������
ISR (TIMER1_COMPA_vect)
{
	if(reading0buf == BUF0_READ)								//�������� �� ������� ������
		OCR0 = read_buf0[PosInBuf++];
	else
		OCR0 = read_buf1[PosInBuf++];
	if(PosInBuf == (BUF_LEN-1)) {									//����� ��������
		if(reading0buf == BUF0_READ)							//�������� ��������������� ����� ��� ������
			buf0_empty = BUF_EMPTY;										
		else
			buf1_empty = BUF_EMPTY;
		reading0buf ^= 1;										//������� �����
		PosInBuf = 0;												//������ ������ �������
	}
}

//-----------------------------------------------------------------------------------
//--------------- ������ � ����������, ���� ����� �� ��������� ----------------------

/************************************************************************/
/* ���������� ������ ������� ��� ���������                              */
/************************************************************************/
void BufferDownload(void){
	if (buf0_empty == BUF_EMPTY)
		buf0_empty = BUF_FULL;
	if (buf1_empty == BUF_EMPTY)
		buf1_empty = BUF_FULL;
	SetTimerTask(BufferDownload, BEEP_READ_PERIOD);
}
/************************************************************************/
/* �������� ������ ������� � ������ ��� ���������������                 */
/************************************************************************/
#define BEEP_SIN_COUNT_BUF	(BUF_LEN/BEEP_SIN_LEN)				//���������� ����������� �������� ������ � ������. 
inline void BufferSineIni(void){
	memcpy_P(read_buf0, &sin_table, BEEP_SIN_LEN);
	memcpy(read_buf0+BEEP_SIN_LEN, read_buf0, BEEP_SIN_LEN);
	memcpy(read_buf1, read_buf0, BUF_LEN);
	buf0_empty = BUF_FULL;
	buf1_empty = BUF_FULL;

	OCR1A = (F_CPU/BEEP_BITRATE)/64;
	START_TMR_BITRT;
	SOUND_ON;
	SetTimerTask(BufferDownload, BEEP_READ_PERIOD);
}

/************************************************************************/
/* ���������� ������� ����� ��� ����� ������                            */
/************************************************************************/
void KeyBeepDownload(void){
	if KeyBeepIsWork{											//���� ������ ��������?
		SetTimerTask(KeyBeepDownload, BEEP_READ_PERIOD);		//��� ���, ���������� �������
	}
	else														//���� ��������
		SoundOff();
}
/************************************************************************/
/* ���������� ��������������� ����� ������                              */
/************************************************************************/
void BeepKeyOff(void){
	KeyBeepEnd;
}

/************************************************************************/
/* ��������� �������� � ��� ��� ����� ������, � ��� �� ��� ����� �����  */
/* ���� ����� �� ������� �������										*/
/************************************************************************/
void BeebKeyboradStart(void){
	BufferSineIni();
	SetTimerTask(KeyBeepDownload, BEEP_READ_PERIOD);			//������ ���������� ������
}

/************************************************************************/
/* �������� ���� � ��� ������ ���� �� ������� ������� ���� �� SD		*/
/* ��������                                                             */
/************************************************************************/
void AlarmBeepContinue(void);									//���������� ������
/************************************************************************/
/* ��������� ������� ��������� �����, �������� ������ ��������			*/
/************************************************************************/
void AlarmBeepStop(void){										//��������� ����
	SOUND_OFF;													//��� ����������� ������ ����� �� ���� ���, �.�. ���� ������ ��������� ����, �� �������� ����� ����� � ������� �������
	STOP_TMR_BITRT;
	if AlarmBeepIsWork											//���� ������� ���������� �������� �� ���������� ������
		SetTimerTask(AlarmBeepContinue, BEEP_OFF_DELAY);		//�������� ����� ��������� ��������
	else if (EachHourIsWork && StrikingClockCount){				//���� ���� ������ ���, �� �������� ���
		StrikingClockCount--;
		SetTimerTask(AlarmBeepContinue, BEEP_OFF_DELAY);		//�������� ����� ��������� ��������
	}
	else{
		SoundOff();												//���� �������� �����������
	}
}
/************************************************************************/
/* ��������� ������� ��������, �������� ������ �����                    */
/************************************************************************/
void AlarmBeepContinue(void){									//���������� ������
	SOUND_ON;
	START_TMR_BITRT;
	SetTimerTask(AlarmBeepStop, BEEP_ON_DELAY);					//��������� ����� ��������� ��������
}
/************************************************************************/
/* ����� ����� ��� �������� SD                                          */
/************************************************************************/
void AlarmBeepStart(void){
	BufferSineIni();											//��������� ��� � �������
	SetTimerTask(AlarmBeepStop, BEEP_ON_DELAY);					//��������� ����� ��������� ��������
}

//-----------------------------------------------------------------------------------
//--------------- ������ � ������ ����� �� SD-�������� ------------------------------

FRESULT FileSoundSet(const char *FileName);
/************************************************************************/
/* ������ ���������� ����� ����� �� SD-��������                         */
/************************************************************************/
inline FRESULT read_part(u08 *buf){
	u08 res;
	WORD rb;													//����������� ����

	res = pf_read(buf, BUF_LEN, &rb);
	if (res != FR_OK)											//������ ������ ���������� ����
		SoundOff();
	return res;
}

void play_file(void){

	if (CurrentChunk > DataChunkSize){							//��������������� ���������
		if (AlarmBeepIsWork)									//���� ��������� ��� �� �������� �� ������ ��������������� �����
			FileSoundSet(ALARM_FILE_NAME);
		else
			if (EachHourIsWork && StrikingClockCount){			//���� ���� ������ ���, �� ������������� ���
				StrikingClockCount--;
				if (FileSoundSet(ONE_HOUR_FILE_NAME) != FR_OK)	//���� ������������ ��� �� ������, ��������� ����
					SoundOff();
			}
			else												//��� ������� ���������
				SoundOff();
		return;
	}
	if(reading0buf == BUF0_READ){
		if (buf1_empty == BUF_EMPTY){
			if (read_part(read_buf1) != FR_OK)
				return;
			buf1_empty = BUF_FULL;
			CurrentChunk += BUF_LEN;
			}
	}
	else{
		if (buf0_empty == BUF_EMPTY){
			if (read_part(read_buf0) != FR_OK)
				return;
			buf0_empty = BUF_FULL;
			CurrentChunk += BUF_LEN;
		}
	}
	SetTimerTask(play_file, BEEP_READ_PERIOD);				//������ ���������� ������
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
//------- ������������ ������� ��������� � �������� ��������� ������ read_buf0, ���������� RET_OK ��� ��� ������
inline FRESULT check_bitrate_and_stereo(void){
	int i;
	unsigned long int bitrate =0;

	if (
		(memcmp(&read_buf0[0], "RIFF", 4) == 0)					//������ ���� ����� RIFF
		&& (memcmp(&read_buf0[8], "WAVE", 4) == 0)				//������ ���� ����� WAVE
		&& (memcmp(&read_buf0[12], "fmt ", 4) == 0)				//������ ���� ������ fmt
		&& (memcmp(&read_buf0[36], "data", 4) == 0)				//������ ���� ������ data
		){
		if(read_buf0[34] != 8)									//������ ���� ����������� ������������
			return FR_NOT_READY;								//�� 8-� ������ ���������
		for (i = 31; i > 27; i--) {								//�������� ���������� �������
			bitrate <<= 8;
			bitrate |= read_buf0[i];
		}
		OCR1A = (F_CPU/bitrate);								//������� ���������� � ��������.
		return FR_OK;
	}
	return FR_NOT_READY;										//��� �� ������ wave
}

/************************************************************************/
/* ��������� ��������, ��� � ������� ��� ��������������� ��������� �����*/
/* FileName - ��� �����, ���������� RET_OK ���� ��� ��������� ��� ���	*/
/* ������ ���� ���-�� ����� �� ���										*/
/************************************************************************/
FRESULT FileSoundSet(const char *FileName){

	FRESULT Ret;
	WORD rb;												//����������� ����

	if (pf_disk_is_mount() != FR_OK){						//����� ��� �� ������������, ������� �� ����������������.
		Ret = pf_mount(&fs);
		if (Ret != FR_OK) return Ret;
	}
	Ret = pf_open(FileName);
	if (Ret != FR_OK) return Ret;
	Ret = pf_lseek (0);
	if (Ret != FR_OK) return Ret;
	Ret = pf_read(&read_buf0, 44, &rb);
	if (Ret != FR_OK) return Ret;
	if (rb != 44) return FR_NOT_OPENED;						//������������ ������ ���������
	Ret = check_bitrate_and_stereo();						//������ ����������
	if (Ret != FR_OK) return Ret;
	memcpy(&DataChunkSize, &read_buf0[40], 4);				//���������� �������
	Ret = pf_lseek(44);										//������ �� ������ ������ �������
	if (Ret != FR_OK) return Ret;
	Ret = read_part(read_buf0);								//������ ���������������
	if (Ret != FR_OK) return Ret;
	buf0_empty = BUF_FULL;
	buf1_empty = BUF_EMPTY;
	CurrentChunk = BUF_LEN;
	PosInBuf = 0;
	reading0buf = BUF0_READ;
	SetTimerTask(play_file, BEEP_READ_PERIOD);				//������ ���������� ������
	START_TMR_BITRT;
	SOUND_ON;
	return FR_OK;
}

//-----------------------------------------------------------------------------------
/************************************************************************/
/* ��������� ����� ������������� ����                                   */
/************************************************************************/
void SoundOn(u08 SoundType){								//�������� ������ ���������� ����

	switch (SoundType)
	{
		case SND_KEY_BEEP:									//���� ������
			if KeyBeepSettingIsOn{							//���� ������� �� ������
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtButton);				//��������� ����������� ���������
#endif				
				KeyBeepBegin;
				BeebKeyboradStart();						//��������� ��� � �������
				SetTimerTask(BeepKeyOff, BEEP_KEY_DELAY);	//������������ �������� ����� ������
			}
			break;
		case SND_ALARM_BEEP:								//���� ����������
			if AlarmBeepIsNoWork{							//��������� ���� ������ ���� �� ��� �� �������
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtAlarm);
#endif
				AlarmBeepBegin;								//���� ����� ���������� �������
				if (FileSoundSet(ALARM_FILE_NAME) != FR_OK)
					AlarmBeepStart();						//�� ������� ������������� ���� � ��������, ��������� ������� �������
			}
			break;
		case SND_EACH_HOUR:									//���� ��������
			if EachHourSettingIsOn{							//���� �������� �������?
#ifdef VOLUME_IS_DIGIT
				VolumeAdjustStart(vtEachHour);
#endif
				EachHourBegin;								//���� ����� �������� �������
				StrikingClockCount = HourToInt(Watch.Hour);	//���������� ������ ��������
				if (StrikingClockCount>12)					//���� ������ ������� �� �������� � ������������� ���
					StrikingClockCount -= 12;
				if (FileSoundSet(EACH_HOUR_FILE_NAME) != FR_OK)
					AlarmBeepStart();						//�� ������� ������������� ���� � ��������, ��������� ������� �������
			}
			break;
	
		case SND_SENSOR_TEST:
		case SND_TEST:										//������ �������� ���� ��� ������������ ���������
#ifdef VOLUME_IS_DIGIT
			if (SoundType == SND_SENSOR_TEST)
				VolumeAdjustStart(vtSensorTest);
			else
				VolumeAdjustStart(vtButton);
#endif
			KeyBeepBegin;
			BeebKeyboradStart();							//��������� ��� � �������
			SetTimerTask(BeepKeyOff, BEEP_KEY_DELAY);		//������������ �������� ����� ������
			break;
		case SND_TEST_EACH:									//���� ����� ��������
#ifdef VOLUME_IS_DIGIT
			VolumeAdjustStart(vtEachHour);
#endif
			EachHourBegin;									//���� ����� �������� �������
			StrikingClockCount = 1;							//���������� ������ ��������
			if (FileSoundSet(EACH_HOUR_FILE_NAME) != FR_OK)
				AlarmBeepStart();							//�� ������� ������������� ���� � ��������, ��������� ������� �������
			break;
		case SND_TEST_ALARM:								//���� ����� ����������
#ifdef VOLUME_IS_DIGIT
			VolumeAdjustStart(vtAlarm);
#endif
			AlarmBeepBegin;									//���� ����� ���������� �������
			if (FileSoundSet(ALARM_FILE_NAME) != FR_OK)
				AlarmBeepStart();							//�� ������� ������������� ���� � ��������, ��������� ������� �������
			break;
		default:
			break;
	}
}

/************************************************************************/
/* ��������� ������                                                     */
/************************************************************************/
void SoundOff(void){
#ifdef VOLUME_IS_DIGIT
	VolumeOff();
#endif
	SOUND_OFF;								//��������� ������ ��� �� ���� OC
	STOP_TMR_BITRT;							//��������� ������ ��������
	buf0_empty = BUF_EMPTY;					//��� ������ �����
	buf1_empty = BUF_EMPTY;
	KeyBeepEnd;
	EachHourEnd;
	AlarmBeepEnd;
}

/************************************************************************/
/* ��������� �������                                                    */
/************************************************************************/
void EachHourSettingSetOff(void){
	EachHourSettingOff;
}

/************************************************************************/
/* ��������-��������� ������ ������ ���                                 */
/************************************************************************/
void EachHourSettingSwitch(void){
	if EachHourSettingIsOn
		EachHourSettingOff;
	else
		EachHourSettingOn;
	if (Refresh != NULL)
		Refresh();
}
/************************************************************************/
/* ��������� ���� �������� ������� ������ ���. 0 - ��������             */
/************************************************************************/
u08 EachHourSettingIsSet(void){
	if EachHourSettingIsOn
		return 1;
	else
		return 0;
}

/************************************************************************/
/* ��������� ���� ������                                                */
/************************************************************************/
void KeyBeepSettingSetOff(void){
	KeyBeepSettingOff;
}

/************************************************************************/
/* ��������-��������� ������ ������� ������                             */
/************************************************************************/
void KeyBeepSettingSwitch(void){
	if KeyBeepSettingIsOn
		KeyBeepSettingOff;
	else
		KeyBeepSettingOn;
	if (Refresh != NULL)
		Refresh();
}

/************************************************************************/
/* ��������� ���� �������� ������. 0 - ���� ������ ��������             */
/************************************************************************/
u08 KeyBeepSettingIsSet(void){
	if KeyBeepSettingIsOn
		return 1;
	else
		return 0;
}

/************************************************************************/
/* ������ ���������� � ������� ������ ������? 0 - ���, 1 - ��           */
/************************************************************************/
u08 AlarmBeepIsSound(void){
	if AlarmBeepIsWork
		return 1;
	else
		return 0;
}

u08 SoundIsBusy(void){
	return (SoundFlag & (Bit(EACH_HOUR_BEEP_WORK) | Bit(KEY_BEEP_WORK) | Bit(ALARM_BEEP_WORK)))?1:0;
}

/************************************************************************/
/* ��������� ������� SD-����� � ����� � ��� ����������� �������� �������*/
/* �������������. ��������� ��������� �� �������. 						*/
/************************************************************************/
void SD_Card_Test(void){
	unsigned char static NumberOfAttempt = 0;					//���������� ������� ������������ ��������
	
	if NoSDCard(){												//����� � ����� ���
		pf_mount(0);											//������������� �����
		NumberOfAttempt = 0;									//������� ������������ 0
		SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);
	}
	else{														//����� � �����
		if (pf_disk_is_mount() != FR_OK){						//����� ��� �� ������������
			if (NumberOfAttempt < (SD_CARD_ATTEMPT_MAX+1)){		//���������� ������� ������������ ����� ��� �� ���������, ����� ������� ��� ��������������
				if (NumberOfAttempt){							//�������� �������� ��� ���������, �������� ������������
					if (pf_mount(&fs) == FR_OK)					//������ ��������������
						SetClockStatus(csSDCardDetect, ssSDCardOk);//������� ������������� ������� ���������
					else										//������������ ��������
						if (NumberOfAttempt == SD_CARD_ATTEMPT_MAX)//������� ���������, ������� ���������
							SetClockStatus(csSDCardDetect, ssSDCardNo);
					SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);	//������������ ������������ ������� ������� �����
				}
				else{
					SetTimerTask(SD_Card_Test, SD_WHITE_PERIOD);
				}
				NumberOfAttempt++;								//������� ���������
			}
		}
		else{
			SetTimerTask(SD_Card_Test, SD_TEST_PERIOD);			//������������ ������������ ������� ������� �����
		}
	}
}

/************************************************************************/
/* ������������� �������� �������                                       */
/************************************************************************/
void SoundIni(void){
	pf_mount(0);												//������������� �����
	timer1_init();												//������������� ������� ��������������� �������
	pwm_init();													//������������� ������� ��������������� ��������� ����������� ������� - ����������� ���
	SD_Card_Test();												//�������� ������� ����� � �����
	KeyBeepSettingOff;
	EachHourSettingOff;
#ifdef VOLUME_IS_DIGIT											//��������� ��������� ��������� ���� �� ����
	VolumeIni();
#endif
}
