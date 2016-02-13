/*
 * Поддержка вывода символов на индикатор в разных режимах.
 * Перекодировка и прочее.
 * ver 1.7
 */ 

#include "Display.h"
#include "Clock.h"
#include "esp8266hal.h"

//****************** Буфер выводимой  строки *****************************
sDisplay DisplayStr;

u08 CellsBuf[DISP_COL];										//Буфер фиксированных знакомест используется в основном для вертикальной смены цифр

//****************** Управление фиксироваными знакоместами ***************
#define	FLASH_PERIOD_MS	500									//Период мигания цифр
const u08 PROGMEM											//Номера колонок начала фиксированных знакомест
	CellsCoord [CELLS_DISP] = {0, 6, 13, 19 };

//****************** Вертикальное скролирование  **************************
#define VERT_SCROLL_STOP		0							//Остановить вертикальное скролирование
#define VERT_SCROLL_START		8							//Начать вертикальное скролирование
u08 CellsVertPos[CELLS_DISP];								//Текущая вертикальная позиция цифры для каждого знакоместа
u08 VertSpeed = VERT_SPEED;									//Скорость вертикального скроллирования

//****************** Горизонтальная прокрутка  **************************
u16 HorizontalSpeed = HORZ_SCROLL_MAX+HORZ_SCROOL_STEP*HORZ_SCROLL_STEPS/2;	//ПО умолчанию средняя скорость прокрутки

//****************** Шрифты **********************************************
#define LEnd					0xAA						//Байт конца описания шрифта. Не должно быть равно 0! т.к. есть нулевые байты в изображении символов. 0xАА выбран потому что очень маловероятно что будет изображение буквы с такой последовательностью бит.

//****************** Шрифты цифр *****************************************
const u08 PROGMEM
	Font [FONT_COUNT_MAX-1][10][FONT_LENGTH] =
	{
		{//-------------------- Шрифт 1
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
		{//-------------------- Шрифт 2
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
		{//-------------------- Шрифт 3
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
		{//-------------------- Шрифт 4
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
		{//-------------------- Шрифт 5
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
		{//-------------------- Шрифт 6
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
/* Шрифт для букв. Байт 0xAA принудительно прекращает вывод буквы       */
/************************************************************************/
const u08 PROGMEM letters [][FONT_LENGTH] =
{
	//-------------------- Шрифт цифр 0
	{ 0x3E, 0x51, 0x49, 0x45, 0x3E },  // 0		От 0 до 9 возможна смена шрифта
	{ 0x00, 0x42, 0x7F, 0x40, 0x00 },  // 1
	{ 0x42, 0x61, 0x51, 0x49, 0x46 },  // 2
	{ 0x21, 0x41, 0x45, 0x4B, 0x31 },  // 3
	{ 0x18, 0x14, 0x12, 0x7F, 0x10 },  // 4
	{ 0x27, 0x45, 0x45, 0x45, 0x39 },  // 5
	{ 0x3C, 0x4A, 0x49, 0x49, 0x30 },  // 6
	{ 0x01, 0x71, 0x09, 0x05, 0x03 },  // 7
	{ 0x36, 0x49, 0x49, 0x49, 0x36 },  // 8
	{ 0x06, 0x49, 0x49, 0x29, 0x1E },  // 9
	//-------------------- Не настраиваемый шрифт букв
	{ 0x7C, 0x12, 0x12, 0x7C, LEnd },  // А  10
	{ 0x7E, 0x4A, 0x4A, 0x32, LEnd },  // Б  11
	{ 0x7E, 0x4A, 0x4A, 0x34, LEnd },  // В  12
	{ 0x7E, 0x02, 0x02, LEnd, 0x00 },  // Г  13
	{ 0x60, 0x3C, 0x22, 0x3E, 0x60 },  // Д  14
	{ 0x7E, 0x4A, 0x4A, LEnd, 0x00 },  // Е  15
	{ 0x66, 0x18, 0x7E, 0x18, 0x66 },  // Ж  16
	{ 0x24, 0x42, 0x4A, 0x34, 0x00 },  // З  17
	{ 0x7E, 0x10, 0x08, 0x7E, LEnd },  // И  18
	{ 0x7C, 0x11, 0x09, 0x7C, LEnd },  // И  19
	{ 0x7E, 0x18, 0x24, 0x42, LEnd },  // К  20
	{ 0x78, 0x04, 0x02, 0x7E, LEnd },  // Л  21
	{ 0x7E, 0x04, 0x08, 0x04, 0x7E },  // М  22
	{ 0x7E, 0x08, 0x08, 0x7E, LEnd },  // Н  23
	{ 0x3C, 0x42, 0x42, 0x3C, LEnd },  // О  24
	{ 0x7E, 0x02, 0x02, 0x7E, LEnd },  // П  25
	{ 0x7E, 0x12, 0x12, 0x0C, LEnd },  // Р  26
	{ 0x3C, 0x42, 0x42, 0x24, LEnd },  // С  27
	{ 0x02, 0x7E, 0x02, LEnd, 0x00 },  // Т  28
	{ 0x4E, 0x50, 0x50, 0x3E, LEnd },  // У  29
	{ 0x0C, 0x12, 0x7E, 0x12, 0x0C },  // Ф  30
	{ 0x66, 0x18, 0x18, 0x66, LEnd },  // Х  31
	{ 0x7E, 0x40, 0x40, 0x7E, 0xC0 },  // Ц  32
	{ 0x0E, 0x10, 0x10, 0x7E, LEnd },  // Ч  33
	{ 0x7E, 0x40, 0x7E, 0x40, 0x7E },  // Ш  34
	{ 0x7E, 0x40, 0x7E, 0x40, 0xFE },  // Щ  35
	{ 0x02, 0x7E, 0x48, 0x30, LEnd },  // Ъ  36
	{ 0x7E, 0x48, 0x30, 0x00, 0x7E },  // Ы  37
	{ 0x7E, 0x48, 0x30, LEnd, 0x00 },  // Ь  38
	{ 0x24, 0x42, 0x4A, 0x3C, LEnd },  // Э  39
	{ 0x7E, 0x08, 0x3C, 0x42, 0x3C },  // Ю  40
	{ 0x4C, 0x32, 0x12, 0x7E, LEnd },  // Я  41
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
	{ 0x00, 0x00, LEnd, 0x00, 0x00 },  // пробел              ZN_SPACE		57
	{ 0x40, LEnd, 0x00, 0x00, 0x00 },  // точка               ZN_DOT		58
	{ 0x00, LEnd, 0x00, 0x00, 0x00 },  // маленький пробел    ZN_SPICA		59
	{ 0x00, 0x00, 0x00, 0x00, 0x00 },  // "полный" пробел     ZN_BLANK		60
	{ 0x08, 0x08, 0x08, 0x08, 0x08 },  // минус               ZN_MINUS		61
	{ 0x08, 0x08, 0x3E, 0x08, 0x08 },  // плюс                ZN_PLUS		62
	{ 0x06, 0x09, 0x09, 0x06, LEnd },  // знак градуса        ZN_CELS		63
	{ 0x04, 0x3F, 0x44, 0x20, LEnd },  // прописная t         ZN__LTL_T		64
	{ 0x08, 0x08, 0x08, LEnd, 0x00 },  // маленький минус     ZN_LTL_MINUS	65
	{ 0x40, 0x40, 0x40, 0x40, 0x40 },  // подчеркивание _     ZN_UNDER_LINE	66
};

//Имена знакомест для специальных символов
#define ZN_SPACE		57				//пробел половинной ширины знакоместа
#define ZN_DOT			58
#define ZN_SPICA		59				//Пробел шириной в одну колонку
#define ZN_BLANK		60				//пробел шириной в одно фиксированное знакоместо
#define ZN_MINUS		61
#define ZN_PLUS			62
#define ZN_CELS			63
#define ZN__LTL_T		64
#define ZN_LTL_MINUS	65
#define ZN_UNDER_LINE	66

/************************************************************************/
/* Таблица подстановки для латинских букв для знакогенератора	       */
/************************************************************************/
const u08 PROGMEM substLat[] =
{
	10,									//A используется изображение русской буквы А
	12,									//B	-//- В
	27,									//C	-//- С
	42,									//D
	15,									//E	-//- Е
	43,									//F
	44,									//G
	23,									//H	-//- Н
	45,									//I
	46,									//J
	20,									//K	-//- К
	47,									//L
	22,									//M	-//- М
	48,									//N
	24,									//O	-//- О
	26,									//P	-//- Р
	49,									//Q
	50,									//R
	51,									//S
	28,									//T	-//- Т
	52,									//U
	53,									//V
	54,									//W
	31,									//X	-//- Х
	55,									//Y
	56									//Z
};

/************************************************************************/
/* Возвращает из знакогенератора столбец Col для символа CharCode	    */
/************************************************************************/
u08 Symbol(u08 CharCode, u08 Col){

	
	if ((CharCode <=0x9) || ((CharCode >= 0x30) && (CharCode <= 0x39))){	//Для цифр особый порядок формирования и шрифта с id !=0 знакогенератор находтся в отдельной таблице
		if (FontIdGet())
			return pgm_read_byte(&Font[FontIdGet()-1][CharCode & 0x0f][Col]);	//Вычитается 1 т.к. нулевой шрифт находится в общем знакогенераторе символов
		else
			return pgm_read_byte(&letters[CharCode & 0x0f][Col]);
	}
	else if (CharCode == S_SPACE /*0x20*/)									//пробел половинной ширины знакоместа, соответствует коду в Win-1251
		return pgm_read_byte(&letters[ZN_SPACE][Col]);
	else if (CharCode == S_PLUS /*0x2b*/)									//Символ '+', соответствует коду в Win-1251
		return pgm_read_byte(&letters[ZN_PLUS][Col]);
	else if (CharCode == S_MINUS /*0x2d*/)									//Символ '-', соответствует коду в Win-1251
		return pgm_read_byte(&letters[ZN_MINUS][Col]);
	else if (CharCode == S_DOT /*0x2e*/)									//Символ '.', соответствует коду в Win-1251
		return pgm_read_byte(&letters[ZN_DOT][Col]);
	else if ((CharCode >= LATIN_BEGIN_CAPITAL) && (CharCode <= LATIN_END_CAPITAL))	//Прописные буквы латинского алфавита. Используется таблица соответствия кодов изображений русских букв и латинских, а также дополнительные картинки латинских букв
		return pgm_read_byte(&letters[pgm_read_byte(&substLat[CharCode-LATIN_BEGIN_CAPITAL])][Col]);
	else if ((CharCode >= LATIN_BEGIN_LOWER) && (CharCode <= LATIN_END_LOWER)){						//Строчные буквы латинского алфавита. Вначале приводятся к кодам прописных букв потом получаются аналогично прописным латинским
		if (CharCode == 0x74)												//Исключая букву t
			return pgm_read_byte(&letters[ZN__LTL_T][Col]);
		else
			return pgm_read_byte(&letters[pgm_read_byte(&substLat[CharCode-LATIN_BEGIN_LOWER])][Col]);
	}
	else if (CharCode == S_LTL_MINUS /*0x96*/)								//Символ маленький минус, соответствует коду в Win-1251
		return pgm_read_byte(&letters[ZN_LTL_MINUS][Col]);
	else if (CharCode == BLANK_SPACE/*0x98*/)								//пробел шириной в одно знакоместо, имеет специальное имя BLANK_SPACE
		return pgm_read_byte(&letters[ZN_BLANK][Col]);
	else if (CharCode == S_SPICA /*0xa0*/)									//пробел шириной в одну колонку, имеет специальное имя S_SPICA
		return pgm_read_byte(&letters[ZN_SPICA][Col]);
	else if (CharCode == S_CELSI /*0xb0*/)									//знак градуса
		return pgm_read_byte(&letters[ZN_CELS][Col]);
	else if ((CharCode >= CYRILLIC_BEGIN_CAPITAL) && (CharCode <= CYRILLIC_END_LOWER)){	//Русские буквы, строчные и прописные
		if (CharCode <= CYRILLIC_END_CAPITAL)								//Прописные
			return pgm_read_byte(&letters[CharCode-182][Col]);
		else
			return pgm_read_byte(&letters[CharCode-214][Col]);				//Строчные
	}
	else																	//Вместо остальных кодов выводится 
		return pgm_read_byte(&letters[ZN_UNDER_LINE][Col]);
}

/************************************************************************/
/* Декодирование символа целиком в указанный буфер.                     */
/* Точки после конца символа гасятся.				                    */
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
/* Флаг мигания цифр                                                    */
/************************************************************************/
void FlashSwitch(void){
	FlashFlipFlop();
	if CreepIsOff(){										//Стоячий режим
		for (u08 i=0; i<CELLS_DISP; i++){					//Мигаем цифрами если надо
			if (FlashDigitIsOn(i) && (CellsVertPos[i] == VERT_SCROLL_STOP)){//Надо мигать цифрой и вертикальное скролирование выключено
				if FlashFlipFlopIsOn(){						//Зажечь знакоместо
					memcpy(DisplayBuf+CellStart(i), CellsBuf+CellStart(i), FONT_LENGTH);//Восстановить знакоместо из вертикального буфера
//					DecodeFull(DisplayBuf+CellStart(i), DisplayStr.Str[i]);
				}
				else{										
					memcpy(CellsBuf+CellStart(i), DisplayBuf+CellStart(i), FONT_LENGTH);//Запомнить знакоместо в вертикальном буфере
					DecodeFull(DisplayBuf+CellStart(i), BLANK_SPACE);//Погасить знакоместо
				}
			}
		}
	}
	SetTimerTask(FlashSwitch, FLASH_PERIOD_MS);
}

/************************************************************************/
/* Управление скоростью бегущей строки - быстрее						*/
/************************************************************************/
void HorizontalAdd(void){
	HorizontalSpeed = ((HorizontalSpeed-HORZ_SCROOL_STEP)<HORZ_SCROLL_MAX)?HORZ_SCROLL_MIN:(HorizontalSpeed-HORZ_SCROOL_STEP);
}

/************************************************************************/
/* за один вызов выводит в буфер экрана окно отображения со cдвигом		*/
/************************************************************************/
void ShowWindow(void){
	u08 i, j;

	if (CreepIsOn() && CreepPrevIsOff()){					//Смена режима со стоячего на бегущий
		DisplayStr.SymbolNum = 0;							//Начинается бег (по граблям :) )
		SymbolPosClear();
		EndDispCreepClear();
		ClearScreen();
	}
	else if (CreepIsOff() && CreepPrevIsOn()){				//Обратная смена с бегущего на стоячий
		ClearScreen();
	}
	CreepPrevSet();											//Запомнить новый режим
	if CreepIsOn(){											//Режим бегущей строки
		for(i=0;i<DISP_COL-1;i++)							//Сдвигается на один пиксел влево
			DisplayBuf[i] = DisplayBuf[i+1];
		if EndDispCreepShow(){								//Идет прокрутка пустого экрана в конце бегущей строки
			EndDispCreepAdd();
			if EndDispCreepIsMax(){							//Конец прокрутки пустого экрана
				EndDispCreepClear();
				DisplayStr.SymbolNum = 0;
				CreepCountDec();							//Уменьшить счетчик прохода бегущей строки
			}
		}
		else{												//Просто прокрутка бегущей строки
			j = Symbol(DisplayStr.Str[DisplayStr.SymbolNum], SymbolPos());
			SymbolPosAdd();
			if (j != LEnd)
				DisplayBuf[DISP_COL-1] = j;					//Выводится если есть что выводить
			if (SymbolPosIsMax() || (j==LEnd)){//Вычисляется следующая позиция
				SymbolPosClear();
				DisplayStr.SymbolNum++;
				if (DisplayStr.Len == DisplayStr.SymbolNum){//Достигнут конец строки
					EndDispCreepAdd();						//Начать прокрутку пустого экрана в конце бегущей строки
				}
				else if(j==LEnd){							//Конец строки еще не достигнут, надо вывести следующий символ
					DisplayBuf[DISP_COL-1] = Symbol(DisplayStr.Str[DisplayStr.SymbolNum], SymbolPos());
					SymbolPosAdd();
				}
			}
		}
	}
	SetTimerTask(ShowWindow, HorizontalSpeed);
}

/************************************************************************/
/* Очистка дисплея                                                      */
/************************************************************************/
void ClearDisplay(void){
	u08 i;
	
	ClearScreen();
	for (i=0;i<=STR_MAX_LENGTH;i++)							//Очистка строки отображения
		DisplayStr.Str[i] = 0;
	DisplayStr.Len = 0;
	DisplayStr.SymbolNum = 0;								//Позицию курсора сбросить
	FlashAllOff();											//Флаги мигания сбросить
	CreepOff();												//Фиксированный вывод
	CreepPrevSet();											//Предыдущий режим сбросить
	SymbolPosClear();										//Позицию в символе сбросить
	EndDispCreepClear();									//Бегущую строку выключить
	for (i=0; i<CELLS_DISP; i++)
		CellsVertPos[i] = VERT_SCROLL_STOP;					//Отменить сдвиг позиций
	for(i=0;i<DISP_COL; i++)								//Очистить буфер вертикального сдвига
		CellsBuf[i] = 0;
}

/************************************************************************/
/* Прокрутка знакоместа сверху вниз, за один вызов на один шаг вниз     */
/************************************************************************/
void ScrollingVert(){
	u08 i, Cell;
	u16 ColBuf;												//Буфер колонки, старшие 8 бит это DisplayBuf, младшие CellsBuf

	if CreepIsOff(){										//Вертикальная прокрутка только в фиксированном режиме
		for (Cell=0; Cell<CELLS_DISP; Cell++){				//перебираем фиксированные позиции
			if (CellsVertPos[Cell] != VERT_SCROLL_STOP){	//Сдвигать эту позицию
				for(i=CellStart(Cell);i<CellStart(Cell)+FONT_LENGTH;i++){
					ColBuf = (CellsBuf[i]<<(DISP_ROW-CellsVertPos[Cell]) & 0x00ff)
								|
							 (((u16)DisplayBuf[i]<<8) & 0xff00);
					ColBuf <<= 1;
					DisplayBuf[i] = (u08)(ColBuf>>8);
				}
				CellsVertPos[Cell]--;
				if (CellsVertPos[Cell]==VERT_SCROLL_STOP){	//Последняя вертикальная прокрутка, еще раз прорисовывается символ
					for (i=CellStart(Cell);i<CellStart(Cell)+FONT_LENGTH;i++)
						DisplayBuf[i] = CellsBuf[i];
				}
			}
		}
	}
	SetTimerTask(ScrollingVert, VertSpeed);
}

/************************************************************************/
/* Добавление символа в строку отображения                              */
/* NumCell - позиция символа в фиксированном режиме. В режиме бегущей   */
/* строки игнорируется					                                */
/************************************************************************/
void sputc(u08 Value, u08 NumCell){
	if CreepIsOff(){										
		if (NumCell > CELLS_DISP-1) return;				//Только с указанием определенного номера ячейки
		if (Value == S_FLASH_ON){						//Включить мигание знакоместа
			FlashDigitOn(NumCell);
		}
		else if (Value == S_FLASH_OFF){					//Выключить мигание знакоместа
			FlashDigitOff(NumCell);
		}
		else {											//Символ для вывода
			DecodeFull(CellsBuf+CellStart(NumCell), Value);
			if ((DisplayStr.Str[NumCell] != Value) && (FlashDigitIsOff(NumCell))){//Смена символа, запускается вертикальное скроллирование для этого знакоместа если для него не включен режим мигания
				CellsVertPos[NumCell] = VERT_SCROLL_START;	//Начать скроллирование
			}
			else{										//Символ не менялся, или первый запуск, просто выводится символ
				CellsVertPos[NumCell] = VERT_SCROLL_STOP;//Прекратить вертикальное скроллирование
				DecodeFull(DisplayBuf+CellStart(NumCell), Value);
			}
			DisplayStr.Str[NumCell] = Value;
		}
	}
	else{
		if ((DisplayStr.Len+1)>STR_MAX_LENGTH) return;	//Некуда выводить
		DisplayStr.Str[DisplayStr.Len++] = Value;		//Поместить символ в буфер строки, декодирование будет в момент отображения
	}
}

/************************************************************************/
/* Вывод точки на дисплей. On=1 включить точку, 0 - выключить           */
/* Работает только в статическом режиме, только до первого обновления   */
/************************************************************************/
//TODO Возможно надо сделать что бы статус точки сохранялся и после обновления экрана
void plot(u08 x, u08 y, u08 On){
	if ((x>=DISP_COL) || (y>=DISP_ROW)) return;
	if CreepIsOff()
		SetBitVal(DisplayBuf[x], y, On);
}

/************************************************************************/
/* Проверка включения точки на дисплее, 1 - включено, 0 -выключено      */
/************************************************************************/
u08 DotIsSet(u08 x, u08 y){
	if ((x>=DISP_COL) || (y>=DISP_ROW)) return 0;
	return BitIsSet(DisplayBuf[x], y)?1:0;
}

/************************************************************************/
/* Переключение на следующий шрифт цифр                                 */
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
/* Инициализация дисплея												*/
/************************************************************************/
void DisplayInit(void){
	
	DispRefreshIni();									//Запуск регенерации дисплея
	SetTimerTask(FlashSwitch, 250);						//Мигание цифр

	FontIdSet(0);
	
	ClearDisplay();										//Очистка дисплея

	ShowWindow();										//Запуск вывода окна бегущей строки.
	ScrollingVert();									//Запуск вертикального скроллирования.
}