/*
 * i2c_ExtTmpr.c
 * Руление датчиком температуры на LM75AD
 * Измерение запускается внешней процедурой. Попытки измерения повторяются до тех пор пока не будет получен результат.
 * v.1.3
 */ 

#include "avrlibtypes.h"
#include "bits_macros.h"
#include "i2c.h"
#include "EERTOS.h"
#include "sensors.h"

/************************************************************************/
/* Операция удачно завершена                                            */
/************************************************************************/
void i2c_OK_ExtTmprFunc(void){
	struct sSensor Sens;											//Ввел сюда структуру, что бы было видно, что здесь работаем со статусом датчика
	Sens.State = 0;
	SensorSetInBus(Sens);											//Сенсор на шине обнаружен
	SensotTypeTemp(Sens);											//Датчик температуры
	if (SetSensor(SENSOR_LM75AD, Sens.State, i2c_Buffer[0]) == SENSOR_SHOW_TEST){
		SetTimerTask(i2c_ExtTmpr_Read, SENSOR_TEST_REPEAT);			//В режиме тестирования запустить новое измерение немедленно
	}
	i2c_Do &= i2c_Free;												//Освобождаем шину
}

/************************************************************************/
/* Ошибка обмена			                                            */
/************************************************************************/
void i2c_Err_ExtTmprFunc(void){
	static u08 Attempt = 3;
	
	if (i2c_Do & i2c_ERR_NA){										//Устройство не отвечает или отсутствует, попробуем еще три раза
		Attempt--;
		if (Attempt)
			SetTimerTask(i2c_ExtTmpr_Read, 2*REFRESH_CLOCK);		//Пробуем повторить операцию
		else{														//После трех неудачных попыток опрос датчика прекращается, но при тестировании повторяется
			struct sSensor Sens;									//Ввел сюда структуру, что бы было видно, что здесь работаем со статусом датчика
			Sens.State = 0;
			SensorNoInBus(Sens);									//Сенсор на шине не обнаружен
			SensotTypeTemp(Sens);									//Датчик температуры
			if (SetSensor(SENSOR_LM75AD, Sens.State, 0) == SENSOR_SHOW_TEST){	//Запомнить значение
				SetTask(i2c_ExtTmpr_Read);							//В режиме тестирования запустить новое измерение немедленно
				Attempt = 0;
			}
		}
	}																
	else	
		SetTimerTask(i2c_ExtTmpr_Read, REFRESH_CLOCK);				//Пробуем повторить операцию
	i2c_Do &= i2c_Free;												//Освобождаем шину
}

/************************************************************************/
/* Чтение регистра из датчика температуры на шине i2c				    */
/************************************************************************/
void i2c_ExtTmpr_Read(void){
	if (i2c_Do & i2c_Busy){											//Шина занята, попытаемся позже
		SetTimerTask(i2c_ExtTmpr_Read, REFRESH_CLOCK);
		return;
	}
	i2c_SlaveAddress = SENSOR_LM75AD;
	i2c_index = 0;													//Писать в буфер с нуля
	i2c_Do &= ~i2c_type_msk;										//Сброс режима
	i2c_ByteCount = i2c_ExtTmprBuffer;								//количество читаемых байт
	i2c_PageAddrCount = 1;											//Длина адреса регистра
	i2c_PageAddrIndex = 0;											//Начальный индекс адреса в буфере адреса
	i2c_PageAddress[0] = extmpradrTEMP;								//Адрес регистра из которого будем читать
	i2c_Do |= i2c_sawsarp;											//Режим чтение данных.

	MasterOutFunc = &i2c_OK_ExtTmprFunc;
	ErrorOutFunc = &i2c_Err_ExtTmprFunc;
	
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;      //Пуск операции
	i2c_Do |= i2c_Busy;
}
