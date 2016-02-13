/*
 * Display.h
	���������� ������� ������� � ������� �����������.
	
		� ����� ������� ���������� ����������� �������� ��� �����������. ������ ������ ������ ���������� ����� ����������� �����������. 
	� ��������� � ������ �������������� ����������� ����� �������� �� ������ ��� ���� ���������
	������������ � ������� ������������ ��������� ����������.
		��������������� �� ����� ��������� ����� ����������� �� ������� ����������� � �������� �������. �.�. ������ ��� ������
	����������� ����� ����������� ��������� ��� ����������� �� �������.
		������� sputc ������������ ���������� � �������� ����������� ������������ ������� � ����� �������.
		��������� �������� ��� ������� sputs ������� � ������ ������������ �������� ������. � ���� ������� ��������� ����� �����������
	�������-���������� ���������, � ������ ������� ���������� ������������� � ���������� Windows-1251 � ����� ����������� ����. � ���������� 
	������ ������� �������� ����������� �� ������� ������������ �� ��������:
		- �� 0x0 �� 0x9 ��������� ��������� �������; 
		- �� 0xa �� 0x2f ��������� ������� �������������, �� ����������� ��������� �����:
			0x11 �������� ������� �������������� ����������. ������ ������������ � ������� sputc ������ � ������� ����������.
			0x12 ��������� ������� �������������� ����������. ������ ������������ � ������� sputc ������ � ������� ����������.
			0x20 ������ ���������� ������ ����������
			0x2b ���� � ������������ � ���������� Win-1251
			0x2d ����� � ������������ � ���������� Win-1251
			0x2e ����� � ������������ � ���������� Win-1251
		- �� 0x30 �� 0x39 ��������� ���������� �������;
		- �� 0x3a �� 0x40 ��������� ������� ������������� (������ ����������� ������� � ASCII)
		- �� 0x41 �� 0x5a ��������� ������� ���������� �������� � ��������� ASCII, �� ������ ������������ ���������� ������� �� ����������� ����� "t", ��� ������ ���������� � ���������
		- �� 0x5b �� 0x60 ��������� ������� �������������, (������ ����������� ������� � ASCII)
			0x5f ���������� ��� ������ ������������� � ������������ � ���������� Win-1251
		- �� 0x61 �� 0x7a ���������� ���������� 0x41-0x5a
		- �� 0x7b �� 0xbf ��������� ������� �������������, �� ����������� �����:
			0x96 ��������� ����� � ������������ � ���������� Win-1251
			0x98 ������ ������� � ���� ����������, ����� ����������� ��� BLANK_SPACE
			0xa0 ������ ������� � ���� �������, ����� ����������� ��� S_SPICA
			0xb0 ���� ������� � ������������ � ���������� Win-1251
		- �� 0xc0 �� 0xff ��������� ���������� ������� �������� �������� � ��������� Win-1251. 
			������ �������� �� 0xc0 �� 0xdf ��������� ���������� ��������� 0xe0 �� 0xff. 
 ver 1.4
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <avr\io.h>
#include <avr\pgmspace.h>
#include <string.h>
#include "bits_macros.h"
#include "avrlibtypes.h"
#include "EERTOS.h"
#include "DisplayHAL.h"

#define STR_MAX_LENGTH				128						//������������ ������ ������
typedef struct {
	u08 Str[STR_MAX_LENGTH];								//������ ������
	u08 Len;												//����� ������ � ������
	u08 SymbolNum;											//������� ��������� ������
	u08 SymbolPosCount;										//������� ��������� � ������� � ������� ������� ������ ����� ������ ������� ������
	u08 FontIdAndFlag;										//����� � ��������������� ����� ������
	u08 Flag;												//����� ������ �����������
} sDisplay;

extern sDisplay DisplayStr;									//����� ������ �������

//SymbolPosAndCount - ���� �� 0,1,2 ������� ������� � �������, 3,4,5,6,7 - ������� ������� ��� ������ ������� ����� � ����� ������� ������
#define SYMBOL_POS_MASK				0b00000111				//����� �������� �������
#define END_DISP_COUNT				0b11111000				//����� �������� ������� ������ � ������ ������� ������
#define END_DISP_ITEM				0b00001000				//������� ����� ��� �������� ������� ������
#define SymbolPos()					(DisplayStr.SymbolPosCount & SYMBOL_POS_MASK)
#define	SymbolPosClear()			do{DisplayStr.SymbolPosCount &= ~SYMBOL_POS_MASK;}while(0)
#define SymbolPosAdd()				do{DisplayStr.SymbolPosCount++;}while(0)	//�.�. �������� ������� �� ��������� FONT_LENGTH=5, �� ����� �������� ����� ������ ������������
#define SymbolPosIsMax()			((DisplayStr.SymbolPosCount & SYMBOL_POS_MASK) == FONT_LENGTH)
#define EndDispCreepClear()			do{DisplayStr.SymbolPosCount &= ~END_DISP_COUNT;}while(0)	//�������� ������� ������� ������ � ����� ������� ������
#define EndDispCreepAdd()			do{DisplayStr.SymbolPosCount += END_DISP_ITEM;}while(0)		//�������� ���� ������� � ������� ������� ������
#define EndDispCreepIsMax()			((DisplayStr.SymbolPosCount & END_DISP_COUNT) == (DISP_COL<<3)) //���� ������ ����� �������?
#define EndDispCreepShow()			(DisplayStr.SymbolPosCount & END_DISP_COUNT)	//������ ����� � ����� ������� ������ ��� ���������?

#if (DISP_COL>31)
	#error "DISP_COL more 31, please counter check in DisplayStr.SymbolPosAndCount"
#endif
#if (FONT_LENGTH>7)
	#error "FONT_LENGTH more 7, please counter check in DisplayStr.SymbolPosAndCount"
#endif

//FontIdAndFlag - �������� ����� . � 0-� �� 3-� ���� id ������ ����. 4-� ��� ���������� ��������� ������ ������ (������������� ��� �������)
#define FONT_ID_MASK				0b00001111				//����� ������ ������
#define SHIFT_PREV_BIT				4						//���� ����������� ������ ������ - �������� ��� ��������������
#define FontIdSet(id)				do{DisplayStr.FontIdAndFlag = ((DisplayStr.FontIdAndFlag & ~FONT_ID_MASK) | ((id) & FONT_ID_MASK));}while(0)
#define FontIdGet()					(DisplayStr.FontIdAndFlag & FONT_ID_MASK)
//Flag - �������� ����� . � 0 �� 2-� ���� ������� ������� ������. 3 - ���� ��������������� � �������� FLASH_PERIOD_MS, � 4 �� 7-� ���� ����� ������� ������������� ���������.
#define	FLASH_BIT					3						//��� �������
#define FLASH_MASK					0b11110000				//���� ����� ������� ������������� ���������
#define FlashFlipFlop()				do{DisplayStr.Flag ^= Bit(FLASH_BIT);}while(0)
#define	FlashFlipFlopIsOn()			BitIsSet(DisplayStr.Flag, FLASH_BIT)
#define FlashAllOff()				do { DisplayStr.Flag &= ~FLASH_MASK;} while (0);
#define FlashDigitOn(Digit)			SetBit(DisplayStr.Flag, (Digit+FLASH_BIT+1))
#define FlashDigitOff(Digit)		ClearBit(DisplayStr.Flag, (Digit+FLASH_BIT+1))
#define FlashDigitIsOn(Digit)		BitIsSet(DisplayStr.Flag, Digit+FLASH_BIT+1)
#define FlashDigitIsOff(Digit)		BitIsClear(DisplayStr.Flag, Digit+FLASH_BIT+1)

#define SHIFT_MASK					0b00000111				//����� ����� �������� ������� ������. 000 - ������������� �����, 111 - ����������� ���������� ������, �� 001 �� 110 ���������� ����������� �������� ������
#define INFINITE_CREEP				0b00000111				//�������� �������� ��� ����������� ������� ������
#define CreepInfinteSet()			do{DisplayStr.Flag |= INFINITE_CREEP;}while(0)		//����������� ����� ��������
#define CreepInfinteIsSet()			(DisplayStr.Flag & SHIFT_MASK) == INFINITE_CREEP)	//����������� ����� �������
#define CreepInfinteIsOff()			(((DisplayStr.Flag & SHIFT_MASK) != INFINITE_CREEP) && ((DisplayStr.Flag & SHIFT_MASK) != 0))	//����������� ����� ��������
#define CreepOn(Count)				do{DisplayStr.Flag = (DisplayStr.Flag & ~SHIFT_MASK) | (Count & SHIFT_MASK);}while(0)	//���������� ���������� �������� ������
#define CreepCountDec()				do{if CreepInfinteIsOff() DisplayStr.Flag--;} while (0) //��������� ������� ������� ������� ������ �� 1
#define CreepCount()				(DisplayStr.Flag & SHIFT_MASK)						//�������� �������� ��������
#define CreepOff()					do{DisplayStr.Flag &= ~SHIFT_MASK;} while (0)		//�������� ������������� �����
#define CreepIsOff()				((DisplayStr.Flag & SHIFT_MASK) == 0)				//������������� �����
#define CreepIsOn()					((DisplayStr.Flag & SHIFT_MASK) != 0)				//����� ������� ������
#define CreepPrevSet()				do{if CreepIsOff() ClearBit(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT); else SetBit(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT);} while(0)
#define CreepPrevIsOn()				BitIsSet(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT)	//���������� ��� ������� �����
#define CreepPrevIsOff()			BitIsClear(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT)//���������� �������������

#define VERT_SPEED					100			//�������� ������������� ������ � ������������� ������, ���� ��� � ��
#define VERT_SPEED_FAST			(VERT_SPEED/10)	//����� ������� ������������ ��������� ��� ���������� ���� � ������ ���������

//************* ������� ���� ����������
#define CELLS_DISP					4			//���������� ��������� �� ������ � ����������� ������
#define DIGIT0						3			//������� ������
#define DIGIT1						2
#define DIGIT2						1
#define DIGIT3						0			//������� ������
#define UNDEF_POS					0xff		//�������������� �������

//****************** ������ ���� **********************************************
#define FONT_LENGTH					5			//������������ ������ ���������� � ������
#define FONT_COUNT_MAX				7			//���������� ������� ����, ������ ������� ����� �������� �������� � ����� ��������������� ��������
#if (FONT_COUNT_MAX>15)
	#error "FONT_COUNT_MAX more than 15, edit the number of bits in DisplayStr.FontIdAndFlag"
#endif

//*************** ����������� ������� ��������� Windows-1251 ����� ���������� � �������� ��� �������
#define S_SPACE						0x20		//������ ���������� ������ ����������
#define S_PLUS						0x2b		//������ '+'
#define S_MINUS						0x2d		//������ '-'
#define S_DOT						0x2e		//������ '.'
#define S_LTL_MINUS					0x96		//������ ��������� �����
#define S_CELSI						0xb0		//���� ������� �
#define S_UNDELINE					0x5f		//������ �������������

//*************** ����������� ������� ��� ��������� Windows-1251
#define S_SPICA						'\xa0'		//��������� ������
#define BLANK_SPACE					0x98		//������ ����������
#define S_FLASH_ON					0x11		//�������� ������� ����������
#define S_FLASH_OFF					0x12		//��������� ������� ����������

//*************** ������� ��������
#define LATIN_BEGIN_CAPITAL			0x41		//��������� ����� ���������� ��������
#define LATIN_END_CAPITAL			0x5a
#define LATIN_BEGIN_LOWER			0x61		//�������� ����� ���������� ��������
#define LATIN_END_LOWER				0x7a
#define CYRILLIC_BEGIN_CAPITAL		0xc0
#define CYRILLIC_END_CAPITAL		0xdf
#define CYRILLIC_BEGIN_LOWER		0xe0
#define CYRILLIC_END_LOWER			0xff

//****************** �������������� ���������  **************************
#define HORZ_SCROLL_MAX			20							//������������ �������� ���������
#define HORZ_SCROLL_MIN			100							//�����������
#define HORZ_SCROLL_STEPS		10							//���������� ����� �����������
#define	HORZ_SCROOL_STEP		((HORZ_SCROLL_MIN-HORZ_SCROLL_MAX)/HORZ_SCROLL_STEPS) //�������� �� ���� ���
#define HORZ_SCROLL_CMD_COUNT	2							//���������� ���� ������ � ������� ���������� esp8266. ������ ���� ���������� �����, ������ - ������� ���

extern const u08 PROGMEM CellsCoord [CELLS_DISP];//������ ������� ������ ������������� ���������
#define CellStart(val)	(pgm_read_byte(&CellsCoord[val]))	//������� � ������ ������ ��� ����� val

void DisplayInit(void);
void sputc(u08 Value, u08 NumCell);				//������� ������ �� �����
void plot(u08 x, u08 y, u08 On);				//���������� ���������� �������
u08 DotIsSet(u08 x, u08 y);						//�������� ��������� ����� �� �������, 1 - ��������, 0 -���������
void ClearDisplay(void);						//������� �������
void ClearVertDisplay(void);					//������� ������������� �������
void NextFont(void);							//���������� ��������� ����� ����
void HorizontalAdd(void);						//��������� �������� ��������������� ������ �� ���� �������
extern u08 VertSpeed;							//�������� ������������� ������
extern u16 HorizontalSpeed;						//�������������� ��������

#endif /* DISPLAY_H_ */