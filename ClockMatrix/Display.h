/*
 * Display.h
	Управление буфером дисплея и буфером отображения.
	
		В буфер дисплея помещаются изображения символов для отображения. Причем первые четыре знакоместа имеют особенности отображения. 
	В частности в режиме фиксированного отображения смена символов на экране для этих знакомест
	производится с помощью вертикальной прокрутки знакоместа.
		Непосредственно на экран выводится буфер отображения по размеру совпадающий с размером дисплея. Т.е. каждый бит буфера
	отображения имеет однозначный светодиод для отображения на дисплее.
		Функция sputc рассчитывает координаты и помещает изображение поступающего символа в буфер дисплея.
		Кодировка символов для функции sputs создана с учетом особенностей решаемой задачи. С одно стороны упростить вывод содержимого
	двоично-десятичных счетчиков, с другой стороны обеспечить совместимость с кодировкой Windows-1251 в части отображения букв. В результате 
	такого подхода значения поступающие на дисплей декодируются по правилам:
		- от 0x0 по 0x9 выводятся арабскими цифрами; 
		- от 0xa до 0x2f выводятся символы подчеркивания, за исключением следующих кодов:
			0x11 включить мигание фиксированного знакоместа. Должно передавается в функцию sputc вместе с номером знакоместа.
			0x12 выключить мигание фиксированного знакоместа. Должно передавается в функцию sputc вместе с номером знакоместа.
			0x20 пробел половинной ширины знакоместа
			0x2b плюс в соответствии с кодировкой Win-1251
			0x2d минус в соответствии с кодировкой Win-1251
			0x2e точка в соответствии с кодировкой Win-1251
		- от 0x30 по 0x39 выводятся арабсвкими цифрами;
		- от 0x3a по 0x40 выводятся символы подчеркивания (всякие специальные символы в ASCII)
		- от 0x41 по 0x5a выводятся буквами латинского алфавита в кодировке ASCII, но всегда отображаются заглавными буквами за исключением буквы "t", она бывает маленькоий и заглавной
		- от 0x5b по 0x60 выводятся символы подчеркивания, (всякие специальные символы в ASCII)
			0x5f собственно сам символ подчеркивания в соответствии с кодировкой Win-1251
		- от 0x61 по 0x7a аналогично промежутку 0x41-0x5a
		- от 0x7b по 0xbf выводятся символы подчеркивания, за исключением кодов:
			0x96 маленький минус в соответствии с кодировкой Win-1251
			0x98 пробел шириной в одно знакоместо, имеет специальное имя BLANK_SPACE
			0xa0 пробел шириной в одну колонку, имеет специальное имя S_SPICA
			0xb0 знак градуса в соответствии с кодировкой Win-1251
		- от 0xc0 по 0xff выводятся заглавными буквами русского алфавита в кодировке Win-1251. 
			Причем интервал от 0xc0 по 0xdf выводится аналогично интервалу 0xe0 по 0xff. 
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

#define STR_MAX_LENGTH				128						//Максимальный размер строки
typedef struct {
	u08 Str[STR_MAX_LENGTH];								//Строка вывода
	u08 Len;												//Длина строки в буфере
	u08 SymbolNum;											//Текущий выводимый символ
	u08 SymbolPosCount;										//Позиция выводимая в символе и счетчик пустого экрана после вывода бегущей строки
	u08 FontIdAndFlag;										//Шрифт и вспомогательные флаги строки
	u08 Flag;												//Флаги строки отображения
} sDisplay;

extern sDisplay DisplayStr;									//Буфер строки дисплея

//SymbolPosAndCount - биты от 0,1,2 счетчик позиций в символе, 3,4,5,6,7 - счетчик позиций для вывода пустого места в конце бегущей строки
#define SYMBOL_POS_MASK				0b00000111				//Маска счетчика позиций
#define END_DISP_COUNT				0b11111000				//Маска счетчика пустого экрана в конеце бегущей строки
#define END_DISP_ITEM				0b00001000				//Единица счета для счетчика пустого экрана
#define SymbolPos()					(DisplayStr.SymbolPosCount & SYMBOL_POS_MASK)
#define	SymbolPosClear()			do{DisplayStr.SymbolPosCount &= ~SYMBOL_POS_MASK;}while(0)
#define SymbolPosAdd()				do{DisplayStr.SymbolPosCount++;}while(0)	//Т.к. значение позиции не превышает FONT_LENGTH=5, то сброс счетчика будет раньше переполнения
#define SymbolPosIsMax()			((DisplayStr.SymbolPosCount & SYMBOL_POS_MASK) == FONT_LENGTH)
#define EndDispCreepClear()			do{DisplayStr.SymbolPosCount &= ~END_DISP_COUNT;}while(0)	//Сбросить счетчик пустого экрана в конце бегущей строки
#define EndDispCreepAdd()			do{DisplayStr.SymbolPosCount += END_DISP_ITEM;}while(0)		//Добавить одну колонку в счетчик пустого экрана
#define EndDispCreepIsMax()			((DisplayStr.SymbolPosCount & END_DISP_COUNT) == (DISP_COL<<3)) //Весь пустой экран выведен?
#define EndDispCreepShow()			(DisplayStr.SymbolPosCount & END_DISP_COUNT)	//Пустой экран в конце бегущей строки еще выводится?

#if (DISP_COL>31)
	#error "DISP_COL more 31, please counter check in DisplayStr.SymbolPosAndCount"
#endif
#if (FONT_LENGTH>7)
	#error "FONT_LENGTH more 7, please counter check in DisplayStr.SymbolPosAndCount"
#endif

//FontIdAndFlag - Значения битов . С 0-й по 3-й биты id шрифта цифр. 4-й бит предыдущее состояние режима строки (фиксированный или бегущий)
#define FONT_ID_MASK				0b00001111				//Маска номера шрифта
#define SHIFT_PREV_BIT				4						//Флаг предыдущего режима строки - бегущего или фиксированного
#define FontIdSet(id)				do{DisplayStr.FontIdAndFlag = ((DisplayStr.FontIdAndFlag & ~FONT_ID_MASK) | ((id) & FONT_ID_MASK));}while(0)
#define FontIdGet()					(DisplayStr.FontIdAndFlag & FONT_ID_MASK)
//Flag - Значения битов . С 0 по 2-й биты счетчик бегущей строки. 3 - флаг переключающийся с частотой FLASH_PERIOD_MS, с 4 по 7-й биты флаги мигания фиксированных знакомест.
#define	FLASH_BIT					3						//Бит мигания
#define FLASH_MASK					0b11110000				//Биты флага мигания фиксированных знакомест
#define FlashFlipFlop()				do{DisplayStr.Flag ^= Bit(FLASH_BIT);}while(0)
#define	FlashFlipFlopIsOn()			BitIsSet(DisplayStr.Flag, FLASH_BIT)
#define FlashAllOff()				do { DisplayStr.Flag &= ~FLASH_MASK;} while (0);
#define FlashDigitOn(Digit)			SetBit(DisplayStr.Flag, (Digit+FLASH_BIT+1))
#define FlashDigitOff(Digit)		ClearBit(DisplayStr.Flag, (Digit+FLASH_BIT+1))
#define FlashDigitIsOn(Digit)		BitIsSet(DisplayStr.Flag, Digit+FLASH_BIT+1)
#define FlashDigitIsOff(Digit)		BitIsClear(DisplayStr.Flag, Digit+FLASH_BIT+1)

#define SHIFT_MASK					0b00000111				//Маска битов счетчика бегущей строки. 000 - фиксированный режим, 111 - бесконечное повторение строки, от 001 до 110 количество необходимых проходов строки
#define INFINITE_CREEP				0b00000111				//Значение счетчика для бесконечной бегущей строки
#define CreepInfinteSet()			do{DisplayStr.Flag |= INFINITE_CREEP;}while(0)		//Бесконечный режим включить
#define CreepInfinteIsSet()			(DisplayStr.Flag & SHIFT_MASK) == INFINITE_CREEP)	//Бесконечный режим включен
#define CreepInfinteIsOff()			(((DisplayStr.Flag & SHIFT_MASK) != INFINITE_CREEP) && ((DisplayStr.Flag & SHIFT_MASK) != 0))	//Бесконечный режим выключен
#define CreepOn(Count)				do{DisplayStr.Flag = (DisplayStr.Flag & ~SHIFT_MASK) | (Count & SHIFT_MASK);}while(0)	//Установить количество проходов строки
#define CreepCountDec()				do{if CreepInfinteIsOff() DisplayStr.Flag--;} while (0) //Уменьшить счетчик прохода бегущей строки на 1
#define CreepCount()				(DisplayStr.Flag & SHIFT_MASK)						//Значение счетчика проходов
#define CreepOff()					do{DisplayStr.Flag &= ~SHIFT_MASK;} while (0)		//Включить фиксированный режим
#define CreepIsOff()				((DisplayStr.Flag & SHIFT_MASK) == 0)				//Фиксированный режим
#define CreepIsOn()					((DisplayStr.Flag & SHIFT_MASK) != 0)				//Режим бегущей строки
#define CreepPrevSet()				do{if CreepIsOff() ClearBit(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT); else SetBit(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT);} while(0)
#define CreepPrevIsOn()				BitIsSet(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT)	//Предыдущий был бегущий режим
#define CreepPrevIsOff()			BitIsClear(DisplayStr.FontIdAndFlag, SHIFT_PREV_BIT)//Предыдущий фиксированный

#define VERT_SPEED					100			//Скорость вертикального сдвига в фиксированном режиме, один шаг в мс
#define VERT_SPEED_FAST			(VERT_SPEED/10)	//Очень быстрая вертикальная прокрутка для обновления цифр в режиме установки

//************* Разряды цифр индикатора
#define CELLS_DISP					4			//Количество знакомест на экране в статическом режиме
#define DIGIT0						3			//Младший разряд
#define DIGIT1						2
#define DIGIT2						1
#define DIGIT3						0			//Старший разряд
#define UNDEF_POS					0xff		//Неопределенная позиция

//****************** Шрифты цифр **********************************************
#define FONT_LENGTH					5			//максимальный размер знакоместа в шрифте
#define FONT_COUNT_MAX				7			//Количество шрифтов цифр, причем нулевой шрифт хранится отдельно в общем знакогенераторе символов
#if (FONT_COUNT_MAX>15)
	#error "FONT_COUNT_MAX more than 15, edit the number of bits in DisplayStr.FontIdAndFlag"
#endif

//*************** Специальные символы кодировки Windows-1251 Здесь приводятся в основном для справки
#define S_SPACE						0x20		//пробел половинной ширины знакоместа
#define S_PLUS						0x2b		//Символ '+'
#define S_MINUS						0x2d		//Символ '-'
#define S_DOT						0x2e		//Символ '.'
#define S_LTL_MINUS					0x96		//Символ маленький минус
#define S_CELSI						0xb0		//знак градуса °
#define S_UNDELINE					0x5f		//Нижнее подчеркивание

//*************** Специальные символы вне кодировки Windows-1251
#define S_SPICA						'\xa0'		//маленький пробел
#define BLANK_SPACE					0x98		//Пустое знакоместо
#define S_FLASH_ON					0x11		//Включить мигание знакоместа
#define S_FLASH_OFF					0x12		//Выключить мигание знакоместа

//*************** Границы символов
#define LATIN_BEGIN_CAPITAL			0x41		//Прописные буквы латинского алфавита
#define LATIN_END_CAPITAL			0x5a
#define LATIN_BEGIN_LOWER			0x61		//Строчные буквы латинского алфавита
#define LATIN_END_LOWER				0x7a
#define CYRILLIC_BEGIN_CAPITAL		0xc0
#define CYRILLIC_END_CAPITAL		0xdf
#define CYRILLIC_BEGIN_LOWER		0xe0
#define CYRILLIC_END_LOWER			0xff

//****************** Горизонтальная прокрутка  **************************
#define HORZ_SCROLL_MAX			20							//Максимальная скорость прокрутки
#define HORZ_SCROLL_MIN			100							//минимальная
#define HORZ_SCROLL_STEPS		10							//Количество шагов регулировки
#define	HORZ_SCROOL_STEP		((HORZ_SCROLL_MIN-HORZ_SCROLL_MAX)/HORZ_SCROLL_STEPS) //Отсчетов на один шаг
#define HORZ_SCROLL_CMD_COUNT	2							//Количество байт данных в команде управления esp8266. Первый байт количество шагов, второй - текущий шаг

extern const u08 PROGMEM CellsCoord [CELLS_DISP];//Номера колонок начала фиксированных знакомест
#define CellStart(val)	(pgm_read_byte(&CellsCoord[val]))	//Позиция в буфере экрана для цифры val

void DisplayInit(void);
void sputc(u08 Value, u08 NumCell);				//Вывести символ на экран
void plot(u08 x, u08 y, u08 On);				//Управление отдельными точками
u08 DotIsSet(u08 x, u08 y);						//Проверка включения точки на дисплее, 1 - включено, 0 -выключено
void ClearDisplay(void);						//Очистка дисплея
void ClearVertDisplay(void);					//Очистка вертикального дисплея
void NextFont(void);							//Установить следующий шрифт цифр
void HorizontalAdd(void);						//Увеличить скорость горизонтального сдвига на одну ступень
extern u08 VertSpeed;							//Скорость вертикального сдвига
extern u16 HorizontalSpeed;						//горизонтальная скорость

#endif /* DISPLAY_H_ */