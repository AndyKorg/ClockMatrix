/*
 * ��������� ������ �������� �� ��������� � ������ �������.
 * ������������� � ������.
 * ver 1.7
 */ 

#include "Display.h"
#include "Clock.h"
#include "esp8266hal.h"

//****************** ����� ���������  ������ *****************************
sDisplay DisplayStr;

u08 CellsBuf[DISP_COL];										//����� ������������� ��������� ������������ � �������� ��� ������������ ����� ����

//****************** ���������� ������������� ������������ ***************
#define	FLASH_PERIOD_MS	500									//������ ������� ����
const u08 PROGMEM											//������ ������� ������ ������������� ���������
	CellsCoord [CELLS_DISP] = {0, 6, 13, 19 };

//****************** ������������ �������������  **************************
#define VERT_SCROLL_STOP		0							//���������� ������������ �������������
#define VERT_SCROLL_START		8							//������ ������������ �������������
u08 CellsVertPos[CELLS_DISP];								//������� ������������ ������� ����� ��� ������� ����������
u08 VertSpeed = VERT_SPEED;									//�������� ������������� ��������������

//****************** �������������� ���������  **************************
u16 HorizontalSpeed = HORZ_SCROLL_MAX+HORZ_SCROOL_STEP*HORZ_SCROLL_STEPS/2;	//�� ��������� ������� �������� ���������

//****************** ������ **********************************************
#define LEnd					0xAA						//���� ����� �������� ������. �� ������ ���� ����� 0! �.�. ���� ������� ����� � ����������� ��������. 0x�� ������ ������ ��� ����� ������������ ��� ����� ����������� ����� � ����� ������������������� ���.

//****************** ������ ���� *****************************************
const u08 PROGMEM
	Font [FONT_COUNT_MAX-1][10][FONT_LENGTH] =
	{
		{//-------------------- ����� 1
			{ 0x7F, 0x7F, 0x41, 0x7F, 0x7F },
			{ 0x00, 0x00, 0x7F, 0x7F, 0x00 },
			{ 0x61, 0x71, 0x59, 0x4F, 0x47 },
			{ 0x41, 0x49, 0x49, 0x7F, 0x7F },
			{ 0x1F, 0x1F, 0x10, 0x7F, 0x7F },
			{ 0x4F, 0x4F, 0x49, 0x79, 0x79 },
			{ 0x7F, 0x7F, 0x49, 0x79, 0x79 },
			{ 0x01, 0x71, 0x79, 0x0F, 0x07 },
			{ 0x7F, 0x7F, 0x49, 0x7F, 0x7F },
			{ 0x5F, 0x5F, 0x51, 0x7F, 0x7F },
		},
		{//-------------------- ����� 2
			{ 0x7F, 0x7F, 0x41, 0x7F, 0x7F },
			{ 0x00, 0x01, 0x7F, 0x7F, 0x00 },
			{ 0x63, 0x73, 0x59, 0x4F, 0x47 },
			{ 0x63, 0x63, 0x49, 0x7F, 0x77 },
			{ 0x1F, 0x1F, 0x10, 0x7F, 0x7F },
			{ 0x6F, 0x6F, 0x49, 0x79, 0x79 },
			{ 0x7F, 0x7F, 0x49, 0x7B, 0x7B },
			{ 0x03, 0x73, 0x79, 0x0F, 0x07 },
			{ 0x77, 0x7F, 0x49, 0x7F, 0x77 },
			{ 0x6F, 0x6F, 0x49, 0x7F, 0x7F },
		},
		{//-------------------- ����� 3
			{ 0x7F, 0x41, 0x41, 0x7F, 0x7F },
			{ 0x00, 0x00, 0x7F, 0x7F, 0x00 },
			{ 0x61, 0x71, 0x59, 0x4F, 0x47 },
			{ 0x41, 0x49, 0x49, 0x7F, 0x7F },
			{ 0x1F, 0x10, 0x10, 0x7F, 0x7F },
			{ 0x4F, 0x49, 0x49, 0x79, 0x79 },
			{ 0x7F, 0x49, 0x49, 0x79, 0x79 },
			{ 0x01, 0x01, 0x01, 0x7F, 0x7F },
			{ 0x7F, 0x49, 0x49, 0x7F, 0x7F },
			{ 0x1F, 0x11, 0x11, 0x7F, 0x7F },
		},
		{//-------------------- ����� 4
			{ 0x7F, 0x41, 0x41, 0x7F, 0x7F },
			{ 0x00, 0x01, 0x7F, 0x7F, 0x00 },
			{ 0x63, 0x71, 0x59, 0x4F, 0x47 },
			{ 0x63, 0x41, 0x49, 0x7F, 0x7F },
			{ 0x1F, 0x10, 0x10, 0x7F, 0x7F },
			{ 0x6F, 0x49, 0x49, 0x79, 0x79 },
			{ 0x7F, 0x49, 0x49, 0x7B, 0x7B },
			{ 0x03, 0x01, 0x01, 0x7F, 0x7F },
			{ 0x7F, 0x49, 0x49, 0x7F, 0x7F },
			{ 0x1F, 0x11, 0x11, 0x7F, 0x7F },
		},
		{//-------------------- ����� 5
			{ 0x3E, 0x7F, 0x41, 0x7F, 0x3E },
			{ 0x00, 0x02, 0x7F, 0x7F, 0x00 },
			{ 0x62, 0x73, 0x59, 0x4F, 0x46 },
			{ 0x22, 0x63, 0x49, 0x7F, 0x36 },
			{ 0x18, 0x14, 0x12, 0x7F, 0x7F },
			{ 0x2F, 0x6F, 0x45, 0x7D, 0x39 },
			{ 0x3E, 0x7F, 0x49, 0x7B, 0x32 },
			{ 0x03, 0x73, 0x79, 0x0F, 0x07 },
			{ 0x36, 0x7F, 0x49, 0x7F, 0x36 },
			{ 0x2E, 0x6F, 0x49, 0x7F, 0x3E },
		},
		{//-------------------- ����� 6
			{ 0x3E, 0x41, 0x41, 0x7F, 0x3E },
			{ 0x00, 0x02, 0x7F, 0x7F, 0x00 },
			{ 0x62, 0x71, 0x59, 0x4F, 0x46 },
			{ 0x22, 0x41, 0x49, 0x7F, 0x36 },
			{ 0x18, 0x14, 0x12, 0x7F, 0x7F },
			{ 0x2F, 0x45, 0x45, 0x7D, 0x39 },
			{ 0x3E, 0x49, 0x49, 0x7B, 0x32 },
			{ 0x03, 0x71, 0x79, 0x0F, 0x07 },
			{ 0x36, 0x49, 0x49, 0x7F, 0x36 },
			{ 0x26, 0x49, 0x49, 0x7F, 0x3E },
		},
	};

/************************************************************************/
/* ����� ��� ����. ���� 0xAA ������������� ���������� ����� �����       */
/************************************************************************/
const u08 PROGMEM letters [][FONT_LENGTH] =
{
	//-------------------- ����� ���� 0
	{ 0x3E, 0x51, 0x49, 0x45, 0x3E },  // 0		�� 0 �� 9 �������� ����� ������
	{ 0x00, 0x42, 0x7F, 0x40, 0x00 },  // 1
	{ 0x42, 0x61, 0x51, 0x49, 0x46 },  // 2
	{ 0x21, 0x41, 0x45, 0x4B, 0x31 },  // 3
	{ 0x18, 0x14, 0x12, 0x7F, 0x10 },  // 4
	{ 0x27, 0x45, 0x45, 0x45, 0x39 },  // 5
	{ 0x3C, 0x4A, 0x49, 0x49, 0x30 },  // 6
	{ 0x01, 0x71, 0x09, 0x05, 0x03 },  // 7
	{ 0x36, 0x49, 0x49, 0x49, 0x36 },  // 8
	{ 0x06, 0x49, 0x49, 0x29, 0x1E },  // 9
	//-------------------- �� ������������� ����� ����
	{ 0x7C, 0x12, 0x12, 0x7C, LEnd },  // �  10
	{ 0x7E, 0x4A, 0x4A, 0x32, LEnd },  // �  11
	{ 0x7E, 0x4A, 0x4A, 0x34, LEnd },  // �  12
	{ 0x7E, 0x02, 0x02, LEnd, 0x00 },  // �  13
	{ 0x60, 0x3C, 0x22, 0x3E, 0x60 },  // �  14
	{ 0x7E, 0x4A, 0x4A, LEnd, 0x00 },  // �  15
	{ 0x66, 0x18, 0x7E, 0x18, 0x66 },  // �  16
	{ 0x24, 0x42, 0x4A, 0x34, 0x00 },  // �  17
	{ 0x7E, 0x10, 0x08, 0x7E, LEnd },  // �  18
	{ 0x7C, 0x11, 0x09, 0x7C, LEnd },  // �  19
	{ 0x7E, 0x18, 0x24, 0x42, LEnd },  // �  20
	{ 0x78, 0x04, 0x02, 0x7E, LEnd },  // �  21
	{ 0x7E, 0x04, 0x08, 0x04, 0x7E },  // �  22
	{ 0x7E, 0x08, 0x08, 0x7E, LEnd },  // �  23
	{ 0x3C, 0x42, 0x42, 0x3C, LEnd },  // �  24
	{ 0x7E, 0x02, 0x02, 0x7E, LEnd },  // �  25
	{ 0x7E, 0x12, 0x12, 0x0C, LEnd },  // �  26
	{ 0x3C, 0x42, 0x42, 0x24, LEnd },  // �  27
	{ 0x02, 0x7E, 0x02, LEnd, 0x00 },  // �  28
	{ 0x4E, 0x50, 0x50, 0x3E, LEnd },  // �  29
	{ 0x0C, 0x12, 0x7E, 0x12, 0x0C },  // �  30
	{ 0x66, 0x18, 0x18, 0x66, LEnd },  // �  31
	{ 0x7E, 0x40, 0x40, 0x7E, 0xC0 },  // �  32
	{ 0x0E, 0x10, 0x10, 0x7E, LEnd },  // �  33
	{ 0x7E, 0x40, 0x7E, 0x40, 0x7E },  // �  34
	{ 0x7E, 0x40, 0x7E, 0x40, 0xFE },  // �  35
	{ 0x02, 0x7E, 0x48, 0x30, LEnd },  // �  36
	{ 0x7E, 0x48, 0x30, 0x00, 0x7E },  // �  37
	{ 0x7E, 0x48, 0x30, LEnd, 0x00 },  // �  38
	{ 0x24, 0x42, 0x4A, 0x3C, LEnd },  // �  39
	{ 0x7E, 0x08, 0x3C, 0x42, 0x3C },  // �  40
	{ 0x4C, 0x32, 0x12, 0x7E, LEnd },  // �  41
	{ 0x7E, 0x42, 0x42, 0x3C, LEnd },  // D	 42
	{ 0x7E, 0x12, 0x02, LEnd, 0x00 },  // F  43
	{ 0x3C, 0x42, 0x52, 0x24, LEnd },  // G  44
	{ 0x7E, LEnd, 0x00, 0x00, 0x00 },  // I  45
	{ 0x30, 0x40, 0x40, 0x3E, LEnd },  // J  46
	{ 0x7E, 0x40, 0x60, LEnd, 0x00 },  // L  47
	{ 0x7E, 0x04, 0x08, 0x10, 0x7E },  // N  48
	{ 0x3C, 0x42, 0x42, 0x3C, 0x40 },  // Q  49
	{ 0x7E, 0x12, 0x32, 0x4C, LEnd },  // R  50
	{ 0x24, 0x4A, 0x4A, 0x30, 0x00 },  // S  51
	{ 0x3E, 0x40, 0x40, 0x3E, 0x00 },  // U  52
	{ 0x02, 0x3C, 0x40, 0x3C, 0x02 },  // V  53
	{ 0x3E, 0x40, 0x30, 0x40, 0x3E },  // W  54
	{ 0x02, 0x0C, 0x70, 0x0C, 0x02 },  // Y  55
	{ 0xC6, 0xA2, 0x92, 0x8A, 0xC6 },  // Z  56
	{ 0x00, 0x00, LEnd, 0x00, 0x00 },  // ������              ZN_SPACE		57
	{ 0x40, LEnd, 0x00, 0x00, 0x00 },  // �����               ZN_DOT		58
	{ 0x00, LEnd, 0x00, 0x00, 0x00 },  // ��������� ������    ZN_SPICA		59
	{ 0x00, 0x00, 0x00, 0x00, 0x00 },  // "������" ������     ZN_BLANK		60
	{ 0x08, 0x08, 0x08, 0x08, 0x08 },  // �����               ZN_MINUS		61
	{ 0x08, 0x08, 0x3E, 0x08, 0x08 },  // ����                ZN_PLUS		62
	{ 0x06, 0x09, 0x09, 0x06, LEnd },  // ���� �������        ZN_CELS		63
	{ 0x04, 0x3F, 0x44, 0x20, LEnd },  // ��������� t         ZN__LTL_T		64
	{ 0x08, 0x08, 0x08, LEnd, 0x00 },  // ��������� �����     ZN_LTL_MINUS	65
	{ 0x40, 0x40, 0x40, 0x40, 0x40 },  // ������������� _     ZN_UNDER_LINE	66
};

//����� ��������� ��� ����������� ��������
#define ZN_SPACE		57				//������ ���������� ������ ����������
#define ZN_DOT			58
#define ZN_SPICA		59				//������ ������� � ���� �������
#define ZN_BLANK		60				//������ ������� � ���� ������������� ����������
#define ZN_MINUS		61
#define ZN_PLUS			62
#define ZN_CELS			63
#define ZN__LTL_T		64
#define ZN_LTL_MINUS	65
#define ZN_UNDER_LINE	66

/************************************************************************/
/* ������� ����������� ��� ��������� ���� ��� ���������������	       */
/************************************************************************/
const u08 PROGMEM substLat[] =
{
	10,									//A ������������ ����������� ������� ����� �
	12,									//B	-//- �
	27,									//C	-//- �
	42,									//D
	15,									//E	-//- �
	43,									//F
	44,									//G
	23,									//H	-//- �
	45,									//I
	46,									//J
	20,									//K	-//- �
	47,									//L
	22,									//M	-//- �
	48,									//N
	24,									//O	-//- �
	26,									//P	-//- �
	49,									//Q
	50,									//R
	51,									//S
	28,									//T	-//- �
	52,									//U
	53,									//V
	54,									//W
	31,									//X	-//- �
	55,									//Y
	56									//Z
};

/************************************************************************/
/* ���������� �� ��������������� ������� Col ��� ������� CharCode	    */
/************************************************************************/
u08 Symbol(u08 CharCode, u08 Col){

	
	if ((CharCode <=0x9) || ((CharCode >= 0x30) && (CharCode <= 0x39))){	//��� ���� ������ ������� ������������ � ������ � id !=0 �������������� �������� � ��������� �������
		if (FontIdGet())
			return pgm_read_byte(&Font[FontIdGet()-1][CharCode & 0x0f][Col]);	//���������� 1 �.�. ������� ����� ��������� � ����� ��������������� ��������
		else
			return pgm_read_byte(&letters[CharCode & 0x0f][Col]);
	}
	else if (CharCode == S_SPACE /*0x20*/)									//������ ���������� ������ ����������, ������������� ���� � Win-1251
		return pgm_read_byte(&letters[ZN_SPACE][Col]);
	else if (CharCode == S_PLUS /*0x2b*/)									//������ '+', ������������� ���� � Win-1251
		return pgm_read_byte(&letters[ZN_PLUS][Col]);
	else if (CharCode == S_MINUS /*0x2d*/)									//������ '-', ������������� ���� � Win-1251
		return pgm_read_byte(&letters[ZN_MINUS][Col]);
	else if (CharCode == S_DOT /*0x2e*/)									//������ '.', ������������� ���� � Win-1251
		return pgm_read_byte(&letters[ZN_DOT][Col]);
	else if ((CharCode >= LATIN_BEGIN_CAPITAL) && (CharCode <= LATIN_END_CAPITAL))	//��������� ����� ���������� ��������. ������������ ������� ������������ ����� ����������� ������� ���� � ���������, � ����� �������������� �������� ��������� ����
		return pgm_read_byte(&letters[pgm_read_byte(&substLat[CharCode-LATIN_BEGIN_CAPITAL])][Col]);
	else if ((CharCode >= LATIN_BEGIN_LOWER) && (CharCode <= LATIN_END_LOWER)){						//�������� ����� ���������� ��������. ������� ���������� � ����� ��������� ���� ����� ���������� ���������� ��������� ���������
		if (CharCode == 0x74)												//�������� ����� t
			return pgm_read_byte(&letters[ZN__LTL_T][Col]);
		else
			return pgm_read_byte(&letters[pgm_read_byte(&substLat[CharCode-LATIN_BEGIN_LOWER])][Col]);
	}
	else if (CharCode == S_LTL_MINUS /*0x96*/)								//������ ��������� �����, ������������� ���� � Win-1251
		return pgm_read_byte(&letters[ZN_LTL_MINUS][Col]);
	else if (CharCode == BLANK_SPACE/*0x98*/)								//������ ������� � ���� ����������, ����� ����������� ��� BLANK_SPACE
		return pgm_read_byte(&letters[ZN_BLANK][Col]);
	else if (CharCode == S_SPICA /*0xa0*/)									//������ ������� � ���� �������, ����� ����������� ��� S_SPICA
		return pgm_read_byte(&letters[ZN_SPICA][Col]);
	else if (CharCode == S_CELSI /*0xb0*/)									//���� �������
		return pgm_read_byte(&letters[ZN_CELS][Col]);
	else if ((CharCode >= CYRILLIC_BEGIN_CAPITAL) && (CharCode <= CYRILLIC_END_LOWER)){	//������� �����, �������� � ���������
		if (CharCode <= CYRILLIC_END_CAPITAL)								//���������
			return pgm_read_byte(&letters[CharCode-182][Col]);
		else
			return pgm_read_byte(&letters[CharCode-214][Col]);				//��������
	}
	else																	//������ ��������� ����� ��������� 
		return pgm_read_byte(&letters[ZN_UNDER_LINE][Col]);
}

/************************************************************************/
/* ������������� ������� ������� � ��������� �����.                     */
/* ����� ����� ����� ������� �������.				                    */
/************************************************************************/
void DecodeFull(u08* pbuf, u08 Value){
	u08 j=0, k = Symbol(Value, j);
	
	while(j<FONT_LENGTH){
		j++;
		if (k == LEnd)
			*pbuf++ = 0;
		else{
			*pbuf++ = k;
			k = Symbol(Value, j);
		}
	}
}

/************************************************************************/
/* ���� ������� ����                                                    */
/************************************************************************/
void FlashSwitch(void){
	FlashFlipFlop();
	if CreepIsOff(){										//������� �����
		for (u08 i=0; i<CELLS_DISP; i++){					//������ ������� ���� ����
			if (FlashDigitIsOn(i) && (CellsVertPos[i] == VERT_SCROLL_STOP)){//���� ������ ������ � ������������ ������������� ���������
				if FlashFlipFlopIsOn(){						//������ ����������
					memcpy(DisplayBuf+CellStart(i), CellsBuf+CellStart(i), FONT_LENGTH);//������������ ���������� �� ������������� ������
//					DecodeFull(DisplayBuf+CellStart(i), DisplayStr.Str[i]);
				}
				else{										
					memcpy(CellsBuf+CellStart(i), DisplayBuf+CellStart(i), FONT_LENGTH);//��������� ���������� � ������������ ������
					DecodeFull(DisplayBuf+CellStart(i), BLANK_SPACE);//�������� ����������
				}
			}
		}
	}
	SetTimerTask(FlashSwitch, FLASH_PERIOD_MS);
}

/************************************************************************/
/* ���������� ��������� ������� ������ - �������						*/
/************************************************************************/
void HorizontalAdd(void){
	HorizontalSpeed = ((HorizontalSpeed-HORZ_SCROOL_STEP)<HORZ_SCROLL_MAX)?HORZ_SCROLL_MIN:(HorizontalSpeed-HORZ_SCROOL_STEP);
}

/************************************************************************/
/* �� ���� ����� ������� � ����� ������ ���� ����������� �� c������		*/
/************************************************************************/
void ShowWindow(void){
	u08 i, j;

	if (CreepIsOn() && CreepPrevIsOff()){					//����� ������ �� �������� �� �������
		DisplayStr.SymbolNum = 0;							//���������� ��� (�� ������� :) )
		SymbolPosClear();
		EndDispCreepClear();
		ClearScreen();
	}
	else if (CreepIsOff() && CreepPrevIsOn()){				//�������� ����� � �������� �� �������
		ClearScreen();
	}
	CreepPrevSet();											//��������� ����� �����
	if CreepIsOn(){											//����� ������� ������
		for(i=0;i<DISP_COL-1;i++)							//���������� �� ���� ������ �����
			DisplayBuf[i] = DisplayBuf[i+1];
		if EndDispCreepShow(){								//���� ��������� ������� ������ � ����� ������� ������
			EndDispCreepAdd();
			if EndDispCreepIsMax(){							//����� ��������� ������� ������
				EndDispCreepClear();
				DisplayStr.SymbolNum = 0;
				CreepCountDec();							//��������� ������� ������� ������� ������
			}
		}
		else{												//������ ��������� ������� ������
			j = Symbol(DisplayStr.Str[DisplayStr.SymbolNum], SymbolPos());
			SymbolPosAdd();
			if (j != LEnd)
				DisplayBuf[DISP_COL-1] = j;					//��������� ���� ���� ��� ��������
			if (SymbolPosIsMax() || (j==LEnd)){//����������� ��������� �������
				SymbolPosClear();
				DisplayStr.SymbolNum++;
				if (DisplayStr.Len == DisplayStr.SymbolNum){//��������� ����� ������
					EndDispCreepAdd();						//������ ��������� ������� ������ � ����� ������� ������
				}
				else if(j==LEnd){							//����� ������ ��� �� ���������, ���� ������� ��������� ������
					DisplayBuf[DISP_COL-1] = Symbol(DisplayStr.Str[DisplayStr.SymbolNum], SymbolPos());
					SymbolPosAdd();
				}
			}
		}
	}
	SetTimerTask(ShowWindow, HorizontalSpeed);
}

/************************************************************************/
/* ������� �������                                                      */
/************************************************************************/
void ClearDisplay(void){
	u08 i;
	
	ClearScreen();
	for (i=0;i<=STR_MAX_LENGTH;i++)							//������� ������ �����������
		DisplayStr.Str[i] = 0;
	DisplayStr.Len = 0;
	DisplayStr.SymbolNum = 0;								//������� ������� ��������
	FlashAllOff();											//����� ������� ��������
	CreepOff();												//������������� �����
	CreepPrevSet();											//���������� ����� ��������
	SymbolPosClear();										//������� � ������� ��������
	EndDispCreepClear();									//������� ������ ���������
	for (i=0; i<CELLS_DISP; i++)
		CellsVertPos[i] = VERT_SCROLL_STOP;					//�������� ����� �������
	for(i=0;i<DISP_COL; i++)								//�������� ����� ������������� ������
		CellsBuf[i] = 0;
}

/************************************************************************/
/* ��������� ���������� ������ ����, �� ���� ����� �� ���� ��� ����     */
/************************************************************************/
void ScrollingVert(){
	u08 i, Cell;
	u16 ColBuf;												//����� �������, ������� 8 ��� ��� DisplayBuf, ������� CellsBuf

	if CreepIsOff(){										//������������ ��������� ������ � ������������� ������
		for (Cell=0; Cell<CELLS_DISP; Cell++){				//���������� ������������� �������
			if (CellsVertPos[Cell] != VERT_SCROLL_STOP){	//�������� ��� �������
				for(i=CellStart(Cell);i<CellStart(Cell)+FONT_LENGTH;i++){
					ColBuf = (CellsBuf[i]<<(DISP_ROW-CellsVertPos[Cell]) & 0x00ff)
								|
							 (((u16)DisplayBuf[i]<<8) & 0xff00);
					ColBuf <<= 1;
					DisplayBuf[i] = (u08)(ColBuf>>8);
				}
				CellsVertPos[Cell]--;
				if (CellsVertPos[Cell]==VERT_SCROLL_STOP){	//��������� ������������ ���������, ��� ��� ��������������� ������
					for (i=CellStart(Cell);i<CellStart(Cell)+FONT_LENGTH;i++)
						DisplayBuf[i] = CellsBuf[i];
				}
			}
		}
	}
	SetTimerTask(ScrollingVert, VertSpeed);
}

/************************************************************************/
/* ���������� ������� � ������ �����������                              */
/* NumCell - ������� ������� � ������������� ������. � ������ �������   */
/* ������ ������������					                                */
/************************************************************************/
void sputc(u08 Value, u08 NumCell){
	if CreepIsOff(){										
		if (NumCell > CELLS_DISP-1) return;				//������ � ��������� ������������� ������ ������
		if (Value == S_FLASH_ON){						//�������� ������� ����������
			FlashDigitOn(NumCell);
		}
		else if (Value == S_FLASH_OFF){					//��������� ������� ����������
			FlashDigitOff(NumCell);
		}
		else {											//������ ��� ������
			DecodeFull(CellsBuf+CellStart(NumCell), Value);
			if ((DisplayStr.Str[NumCell] != Value) && (FlashDigitIsOff(NumCell))){//����� �������, ����������� ������������ �������������� ��� ����� ���������� ���� ��� ���� �� ������� ����� �������
				CellsVertPos[NumCell] = VERT_SCROLL_START;	//������ ��������������
			}
			else{										//������ �� �������, ��� ������ ������, ������ ��������� ������
				CellsVertPos[NumCell] = VERT_SCROLL_STOP;//���������� ������������ ��������������
				DecodeFull(DisplayBuf+CellStart(NumCell), Value);
			}
			DisplayStr.Str[NumCell] = Value;
		}
	}
	else{
		if ((DisplayStr.Len+1)>STR_MAX_LENGTH) return;	//������ ��������
		DisplayStr.Str[DisplayStr.Len++] = Value;		//��������� ������ � ����� ������, ������������� ����� � ������ �����������
	}
}

/************************************************************************/
/* ����� ����� �� �������. On=1 �������� �����, 0 - ���������           */
/* �������� ������ � ����������� ������, ������ �� ������� ����������   */
/************************************************************************/
//TODO �������� ���� ������� ��� �� ������ ����� ���������� � ����� ���������� ������
void plot(u08 x, u08 y, u08 On){
	if ((x>=DISP_COL) || (y>=DISP_ROW)) return;
	if CreepIsOff()
		SetBitVal(DisplayBuf[x], y, On);
}

/************************************************************************/
/* �������� ��������� ����� �� �������, 1 - ��������, 0 -���������      */
/************************************************************************/
u08 DotIsSet(u08 x, u08 y){
	if ((x>=DISP_COL) || (y>=DISP_ROW)) return 0;
	return BitIsSet(DisplayBuf[x], y)?1:0;
}

/************************************************************************/
/* ������������ �� ��������� ����� ����                                 */
/************************************************************************/
void NextFont(void){
	if (FontIdGet()+1 == FONT_COUNT_MAX)
		FontIdSet(0);
	else
		FontIdSet(FontIdGet()+1);
	if (Refresh != NULL){
		ClearDisplay();
		Refresh();
	}
	u08 i = FontIdGet();
	espUartTx(ClkWrite(CLK_FONT), &i, 1);
}

/************************************************************************/
/* ������������� �������												*/
/************************************************************************/
void DisplayInit(void){
	
	DispRefreshIni();									//������ ����������� �������
	SetTimerTask(FlashSwitch, 250);						//������� ����

	FontIdSet(0);
	
	ClearDisplay();										//������� �������

	ShowWindow();										//������ ������ ���� ������� ������.
	ScrollingVert();									//������ ������������� ��������������.
}