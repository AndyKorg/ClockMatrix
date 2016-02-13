/*
 * ќбслуживание клавиатуры 
 */ 


#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <avr/io.h>
#include "avrlibtypes.h"
#include "Clock.h"

void InitKeyboard(void);
void TimeoutReturnToClock(u08 Minuts);	//—брос часов в основной режим по таймауту либо установка интервала таймаута

#endif /* KEYBOARD_H_ */