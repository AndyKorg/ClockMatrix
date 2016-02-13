/*
 * Самый нижний уровень вывода буфера экрана на индикатор.
 * ver. 1.6
 */ 
//TODO:Подключить механизм контроля работы встроенный в MBI5039
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stddef.h>
#include "avrlibtypes.h"
#include "bits_macros.h"
#include "Clock.h"
#include "DisplayHAL.h"
#include "EERTOS.h"
#ifdef IR_SAMSUNG_ONLY
	#include "IRreciveHAL.h"
#endif

volatile u08 Pallor = 0;								//Бледность цифр. 0 - самая яркая, рассчитывается для текущей освещенности

u08 DisplayBuf[DISP_COL];								//Буфер отображаемого экрана

//Вывод байта во внешние регистры
void OutByteSPI(u08 Value){
	u08 i;

	for (i=0; i<8; i++){
		SetBitVal(DISP_PORT_SPI, DISP_SPI_MOSI, (Value & 1)?1:0);//Вывести бит
		Value >>= 1;
		clckLowLED;										//Подготовить вывод CLK
		clckHightLED;
		clckLowLED;
	}
}

/************************************************************************/
/* Задержка для того что бы светодиоды успели погореть некоторое время. */
/* Вынесена для уменьшения объема исходника								*/
/************************************************************************/
inline u08 DelayForBrightness(u08 *Value){
	
	if (IRPeriodEvent != NULL)							//Обработать событие для ИК датчика если оно есть
		IRPeriodEvent();
	
	if ((*Value) != 0){											
		(*Value)--;
		if ((*Value) <= Pallor)							//Уменьшить яркость
			DisplayOff;
		else
			DisplayOn;
		return 1;
	}
	return 0;
}

/************************************************************************/
/* Обработка службы RTOS                                                */
/************************************************************************/
#define DelayRTOS	33									//Делитель для получения 1 мс интервалов
void TimeServiceRTOS(void){
	static u08 CountTime = DelayRTOS;
	
	if (CountTime)
		CountTime--;
	else{
		CountTime = DelayRTOS;
		TimerService();
	}
}
	
/************************************************************************/
/* Вывод на регистры дисплея. Используется								*/
/* прерывание от 32 кГц													*/
/************************************************************************/
#if defined(LED_MATRIX_COMMON_ROW)

//Для анодов в строках
ISR(DISP_INT_VECTOR_32HZ){

	static u08
		Delay = DELAY_DISP,
#if defined(SHIFT_REG_TYPE_HC595)
#error "not ready yet"
#elif defined(SHIFT_REG_TYPE_MBI50XX)
		row1=0, row2=0, row3=0,
#endif
		ComRow = 0,
		Status = 0;
		
	TimeServiceRTOS();									//Следующая задача RTOS
	
	if (DelayForBrightness(&Delay))						//Задержка отработана?
		return;
	
	if (Status == 0)									//Шаг 0. Начать подготовку байта
		row1=0,row2=0,row3=0;
	else if ((Status>=1) && (Status<=8)){				//Шаги с 1 по 8 для подготовки строк
		row1 <<= 1;
		row2 <<= 1;
		row3 <<= 1;
		if BitIsSet(DisplayBuf[Status-1], ComRow) row1 |= 1;
		if BitIsSet(DisplayBuf[Status-1+8], ComRow) row2 |= 1;
		if BitIsSet(DisplayBuf[Status-1+16], ComRow) row3 |= 1;
	}
#if defined(SHIFT_REG_TYPE_MBI50XX)						//Пока только для MBI50XX
	else if (Status == 9)								//Шаг 9. Включение соответствующей строки
		OutByteSPI(Bit(ComRow));
	else if (Status == 10)								//Шаг 10. Включение строки в индикаторе 1
		OutByteSPI(row1);
	else if (Status == 11)								//Шаг 11. Включение строки в индикаторе 2
		OutByteSPI(row2);
	else if (Status == DISP_STEP_OUT){					//Шаг 12. Включение строки в индикаторе 3 и собственно включение светодиодов
		OutByteSPI(row3);
		upldHightLED;
		upldLowLED;
		ComRow++;										//Подготовка следующего цикла
		if (ComRow == DISP_ROW)
			ComRow = 0;
		Delay = DELAY_DISP-DISP_STEP_OUT;
		Status = 0xff;
	}
#endif
	Status++;
}
#elif defined(LED_MATRIX_COMMON_COLUMN)

//Для анодов в столбцах
ISR(DISP_INT_VECTOR_32HZ){

	static u08
		Delay = DELAY_DISP,
		ComCol = 0,										//Счетчик общих колонок
		Status = 0;

	TimeServiceRTOS();									//Следующая задача RTOS

	if (DelayForBrightness(&Delay))						//Задержка отработана?
		return;
	
	#define ByteOfCol(Nm)	(DisplayBuf[(DISP_COL-1)-(ComCol+(8*Nm))])	//Байт буфера дисплея для индикатора Nm и колонки Col
#ifdef SHIFT_REG_TYPE_HC595								//Для схемы на регистрах HC595
	#define DispByte1	ByteOfCol(2)					//Байт 1 для этого типа внешних регистров
	#define DispByte2	ByteOfCol(1)					//Байт 2 для этого типа внешних регистров
	#define DispByte3	ByteOfCol(0)					//Байт 3 для этого типа внешних регистров
	#define DispByte4	(~Bit(ComCol))					//Байт 4 для этого типа внешних регистров
#elif defined(SHIFT_REG_TYPE_MBI50XX)					//Для схемы на MBI50XX
	#define DispByte1	Bit(ComCol)						//Байт 1 для этого типа внешних регистров
	#define DispByte2	ByteOfCol(2)					//Байт 2 для этого типа внешних регистров
	#define DispByte3	ByteOfCol(1)					//Байт 3 для этого типа внешних регистров
	#define DispByte4	ByteOfCol(0)					//Байт 4 для этого типа внешних регистров
#else
	#error "Undefined shift register type"
#endif

	if (Status == 0){
#ifdef SHIFT_REG_TYPE_HC595								//Для схемы на регистрах HC595
		upldLowLED;										//Подготовить линию загрузки в регистр
#endif
		OutByteSPI(DispByte1);							//Вывести байт 1
	}
	if (Status == 1)
		OutByteSPI(DispByte2);
	if (Status == 2)
		OutByteSPI(DispByte3);
	if (Status == DISP_STEP_OUT){
		OutByteSPI(DispByte4);
#ifdef SHIFT_REG_TYPE_HC595								//Для схемы на регистрах HC595
		upldHightLED;									//Переключить выводы регистров
#elif defined(SHIFT_REG_TYPE_MBI50XX)					//Для схемы на MBI50XX
		upldHightLED;
		upldLowLED;
#else
	#error "Undefined shift register type"
#endif
		Status = 0xff;
		Delay = DELAY_DISP-DISP_STEP_OUT;
		ComCol++;										//Следующая колонка
		if (ComCol == DISP_COL_DOT)						//Все колонки пройдены. начинаем с начала
			ComCol = 0;
	}													//Вывести подготовленное подключение на индикатор
	Status++;											//Следующий шаг подготовки колонок
}
#else
#error "Shift register undefined"
#endif

/************************************************************************/
/* Измерение освещенности часов и регулировка яркости                   */
/************************************************************************/
void IllumMeasure(void){
	ADMUX = (0<<REFS1) | (1<<REFS0) |		//Опорное напряжение равно напряжению питания
			 ILLUM_PORT |					//Измерять освещенность на порту фоторезистора
			 (1<<ADLAR);					//Выравнивание результата влево
	ADCSRA = (1<<ADEN)	|					//Разрешить АЦП
			 (1<<ADSC)  |					//Старт преобразования
			 (1<<ADIE)	|					//Разрешить прерывание по окончании преобразования
			 (0<<ADATE) |					//Запретить автостарт
			PrescalerADC;
	SetTimerTask(IllumMeasure, 1000);		//Проверить освещенность через 1 секунду
}

/************************************************************************/
/* Результат измерения освещенности										*/
/************************************************************************/
ISR(ADC_vect){
	u08 i = (u08)(ADC>>8);

#if (DISP_BOUNDARY_LVL >= (DELAY_DISP/DELAY_INTERVAL))
#error "The boundary of brightness control shall be less than" (DELAY_DISP/DELAY_INTERVAL)
#endif
	
	if (ClockStatus == csClock){			//Регулировка яркости действует только в режиме нормального отображения
		if (i<=DISP_BOUNDARY_LVL)
			Pallor = DELAY_DISP-i-(DISP_STEP_OUT+3);//Вычитается DISP_STEP_OUT для обеспечения минимально яркости дисплея при котором он еще не начинает мерцать
		else if (i<=(DELAY_DISP/DELAY_INTERVAL))
			Pallor = DELAY_DISP-(i*DELAY_INTERVAL);
		else
			Pallor = 0;
	}
	else
		Pallor = 0;
}

/************************************************************************/
/* Очистка экранного буфера                                             */
/************************************************************************/
void ClearScreen(void){
	u08 i;
	
	for (i=0;i<=DISP_COL-1;i++)
	DisplayBuf[i] = 0;
}

void DispRefreshIni(void)
{
	DISP_DDR_SPI |= (1<<DISP_SPI_SCK) | (1<<DISP_SPI_MOSI) | (1<<DISP_SPI_SS);
	DISP_OFF_DDR |= (1<<DISP_OFF_PIN);
	DisplayOn;								//Включить вывод на дисплей 

#ifdef SHIFT_REG_TYPE_HC595
	DISP_CLR_DDR |= (1<<DISP_CLR_PIN);
	DisplayClrReg;							//Очистить регистры TPIC
	DisplayNoClrReg;
	upldHightLED;
	clckHightLED;
	upldLowLED;								//Подготовить линию загрузки в регистр
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	clckLowLED;								//Подготовить вывод CLK
#else
#error "Shift register undefined"
#endif

	OutByteSPI(0);
	OutByteSPI(0);
	OutByteSPI(0);
	OutByteSPI(0);
#ifdef SHIFT_REG_TYPE_HC595
	upldHightLED;							//Вывести подготовленное подключение на индикатор
#elif defined(SHIFT_REG_TYPE_MBI50XX)
	upldHightLED;
	upldLowLED;
#else
#error "Shift register undefined"
#endif
	ClearScreen();

	MCUCR |= (1<<ISC11) | (1<<ISC10);		//Инициализация прерывания INT1 от фронта импульса
	GICR |= (1<<INT1);						//Разрешить прерывание от INT1
	
	IllumMeasure();							//Запуск измерителя освещенности
}
