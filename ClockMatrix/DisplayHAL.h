/*
 * Самый нижний уровень вывода буфера экрана на индикатор.
 * Используется внешнее прерывание от генератора 32 768 Гц для 
 * обновления дисплея, т.к. планировщик не успевает обработать вывод на дисплей при воспроизведении звука. 
 * Сигнал от генератора подан на
 * ногу внешнего прерывания INT1
 * ver.1.3	
 */ 


#ifndef DISPLAYHAL_H_
#define DISPLAYHAL_H_

#include "Clock.h"
#include "IRrecive.h"							//Подключено поскольку используется в отсчете интервала

#if (defined(SHIFT_REG_TYPE_HC595) && defined(SHIFT_REG_TYPE_MBI50XX))
#error "one shall be defined only or SHIFT_REG_TYPE_HC595 or SHIFT_REG_TYPE_MBI50XX"
#endif

#if (defined(LED_MATRIX_COMMON_ROW) && defined(LED_MATRIX_COMMON_COLUMN))
#error "one shall be defined only or LED_MATRIX_COMMON_ROW or LED_MATRIX_COMMON_COLUMN"
#endif

extern u08 DisplayBuf[DISP_COL];				//Буфер отображаемого экрана

void DispRefreshIni(void);						//Инициализация регенерации дисплея
void ClearScreen(void);							//Очистка буфера экрана

#endif /* DISPLAYHAL_H_ */