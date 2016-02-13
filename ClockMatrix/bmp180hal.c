/*
 * Для работы динамически захватывается память с указателем в BMP180Buf
 * Расчеты истинного давления взяты из API https://github.com/BoschSensortec/BMP180_driver
 */ 

#include <stdlib.h>
#include "avrlibtypes.h"
#include "EERTOS.h"
#include "IIC_ultimate.h"
#include "bmp180hal.h"

#ifndef DEBUG
#define DEBUG
#warning "Debug ON!!"
#endif
#ifdef DEBUG
#include "_usart.h"
#endif

u08 BMP180_Flag;											//Счетчик и флаги. 0,1,2,3,4 биты счетчик адреса регистров коэфф. 
#define BMP180_ADR_COFF_MASK		0b00011111
#if (BMP180_COFF_COUNT > BMP180_ADR_COFF_MASK)
	#error "BMP180_ADR_COFF_MASK less then BMP180_COFF_COUNT"
#endif
#define bmp180_AddrCoffIsEnd()		((BMP180_Flag & BMP180_ADR_COFF_MASK) == BMP180_COFF_COUNT)	//Достигнут конечный адрес. 
#define bmp180_AdrCoffCount()		(BMP180_Flag & BMP180_ADR_COFF_MASK) //значение счетчика 

#define BMP180_TWO_EXPONENT_16		16						//2 в 16-й степени
#define BMP180_TWO_EXPONENT_15		15
#define BMP180_TWO_EXPONENT_13		13
#define BMP180_TWO_EXPONENT_12		12
#define BMP180_TWO_EXPONENT_11		11
#define BMP180_TWO_EXPONENT_8		8
#define BMP180_TWO_EXPONENT_4		4
#define BMP180_TWO_EXPONENT_2		2
#define BMP180_TWO_EXPONENT_1		1

s16 BMP180_Temper;
s32 BMP180_Pressure;
u08* BMP180Buf;												//Буфер для калибровочных коэффициентов

/************************************************************************/
/* Обработка ошибки шины I2c                                            */
/************************************************************************/
void BMP180_ReadErr(void){
	if (BMP180Buf != NULL)
		free(BMP180Buf);
	i2c_Do |= i2c_Free;
}

/************************************************************************/
/* Заполнение параметров шины I2C                                       */
/************************************************************************/
void BMP180_I2C_Fill(u08 i2cCMD, u08 Adr, u08 Cmd, u08 ByteCount, IIC_F OKFunc){
	i2c_Do |= i2c_Busy;												//Занять шину
	if (i2cCMD == i2c_sawp){										//Запись команды
		i2c_Buffer[0] = Adr;										//Адрес в режиме записи передается как обычный байт
		i2c_Buffer[1] = Cmd;
		i2c_ByteCount = ByteCount+1;								
	}
	else{
		i2c_ByteCount = ByteCount;									//количество байт данных
	}
	i2c_Do &= ~i2c_type_msk;										//Сброс режима
	i2c_Do |= i2cCMD;
	i2c_SlaveAddress = BMP180_ADDRESS;
	i2c_index = 0;													//Писать в буфер с нуля
	i2c_PageAddress[0] = Adr;										//Адрес регистра
	i2c_PageAddrCount = 1;											//Длина адреса 1 байт
	i2c_PageAddrIndex = 0;											//Адрес с нуля
	MasterOutFunc = OKFunc;
	ErrorOutFunc = BMP180_ReadErr;
	TWCR = 1<<TWSTA|0<<TWSTO|1<<TWINT|0<<TWEA|1<<TWEN|1<<TWIE;		//Пуск операции
}

inline s16 RawToSign16(u08 msb, u08 lsb){
	return 
		(s16)
			(
				(
					(
						(s32)
						(
							(s08)msb
						)
					)<<8
				)
				|
				lsb
			);
}

inline u16 RawToUnsign16(u08 msb, u08 lsb){
	return
		(u16)
			(
				(
					(
						(u32)
						(
							(u08)msb
						)
					)<<8
				)
				| 
				lsb
			);
}

/************************************************************************/
/* Расчет давления с учетом поправочных коэффициентов					*/
/************************************************************************/
void BMP180_PressureCalc(void){

	struct {
		s16 ac1;	//Калибровочные коэфф.
		s16 ac2;
		s16 ac3;
		u16 ac4;
		u16 ac5;
		u16 ac6;
		s16 b1;
		s16 b2;
		s16 mb;
		s16 mc;
		s16 md;
	} calib_param;

	//Преобразование в нужный формат чисел
	calib_param.ac1 = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_AC1), *(BMP180Buf+BMP180_COEFFCNT_S_AC1+1));
	calib_param.ac2 = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_AC2), *(BMP180Buf+BMP180_COEFFCNT_S_AC2+1));
	calib_param.ac3 = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_AC3), *(BMP180Buf+BMP180_COEFFCNT_S_AC3+1));
	calib_param.ac4 = RawToUnsign16(*(BMP180Buf+BMP180_COEFFCNT_U_AC4), *(BMP180Buf+BMP180_COEFFCNT_U_AC4+1));
	calib_param.ac5 = RawToUnsign16(*(BMP180Buf+BMP180_COEFFCNT_U_AC5), *(BMP180Buf+BMP180_COEFFCNT_U_AC5+1));
	calib_param.ac6 = RawToUnsign16(*(BMP180Buf+BMP180_COEFFCNT_U_AC6), *(BMP180Buf+BMP180_COEFFCNT_U_AC6+1));
	calib_param.b1 = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_B1), *(BMP180Buf+BMP180_COEFFCNT_S_B1+1));
	calib_param.b2 = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_B2), *(BMP180Buf+BMP180_COEFFCNT_S_B2+1));
	calib_param.mb = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_MB), *(BMP180Buf+BMP180_COEFFCNT_S_MB+1));
	calib_param.mc = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_MC), *(BMP180Buf+BMP180_COEFFCNT_S_MC+1));
	calib_param.md = RawToSign16(*(BMP180Buf+BMP180_COEFFCNT_S_MD), *(BMP180Buf+BMP180_COEFFCNT_S_MD+1));
	free(BMP180Buf);												//Буфер больше не нужен

/*	printf("ac1=%d\n",calib_param.ac1);
	printf("ac2=%d\n",calib_param.ac2);
	printf("ac3=%d\n",calib_param.ac3);
	printf("ac4=%d\n",calib_param.ac4);
	printf("ac5=%d\n",calib_param.ac5);
	printf("ac6=%d\n",calib_param.ac6);
	printf("b1=%d\n",calib_param.b1);
	printf("b2=%d\n",calib_param.b2);
	printf("mb=%d\n",calib_param.mb);
	printf("mc=%d\n",calib_param.mc);
	printf("md=%d\n",calib_param.md);
	printf("uncom t=%d \n",BMP180_Temper);
	printf("uncom u=%d ",(s16)BMP180_Pressure);*/
	
	s32 v_x1_s32, v_x2_s32 = 0,	v_x3_s32, b5, v_b3_s32, v_b6_s32 = 0, v_pressure_s32;
	u32 v_b4_u32, v_b7_u32 = 0;
	//--------- расчет температуры
	v_x1_s32 =	(((s32) BMP180_Temper - (s32) calib_param.ac6) * (s32) calib_param.ac5)>> BMP180_TWO_EXPONENT_15;
	if ((v_x1_s32 != 0) || (calib_param.md	!= 0)){	//Проверка перед делением на 0
		v_x2_s32 = ((s32) calib_param.mc << BMP180_TWO_EXPONENT_11) / (v_x1_s32 + calib_param.md);
		b5 = v_x1_s32 + v_x2_s32;
		BMP180_Temper = ((b5 +8)>> BMP180_TWO_EXPONENT_4);
		//--------- расчет давления
		v_x2_s32 = 0;												
		v_b6_s32 = b5 - 4000;
		//Расчет b3
		v_x1_s32 = (v_b6_s32*v_b6_s32)>>BMP180_TWO_EXPONENT_12;
		v_x1_s32 *= calib_param.b2;
		v_x1_s32 >>= BMP180_TWO_EXPONENT_11;
		v_x2_s32 = calib_param.ac2 * v_b6_s32;
		v_x2_s32 >>= BMP180_TWO_EXPONENT_11;
		v_x3_s32 = v_x1_s32 + v_x2_s32;
		v_b3_s32 = (((((s32)calib_param.ac1)*4 + v_x3_s32) <<BMP180_OSS) + 2) >> BMP180_TWO_EXPONENT_2;
		//Расчет b4
		v_x1_s32 = (calib_param.ac3 * v_b6_s32)	>> BMP180_TWO_EXPONENT_13;	
		v_x2_s32 = (calib_param.b1 * ((v_b6_s32 * v_b6_s32) >> BMP180_TWO_EXPONENT_12))	>> BMP180_TWO_EXPONENT_16;
		v_x3_s32 = ((v_x1_s32 + v_x2_s32) + 2)>> BMP180_TWO_EXPONENT_2;
		v_b4_u32 = (calib_param.ac4 * (u32)	(v_x3_s32 + 32768)) >> BMP180_TWO_EXPONENT_15;
		v_b7_u32 = ((u32)(BMP180_Pressure - v_b3_s32) *	(50000 >> BMP180_OSS));
		if (v_b4_u32 != 0){
			if (v_b7_u32 < 0x80000000) v_pressure_s32 = (v_b7_u32 << BMP180_TWO_EXPONENT_1) / v_b4_u32;
			else v_pressure_s32 = (v_b7_u32 / v_b4_u32) << BMP180_TWO_EXPONENT_1;
			v_x1_s32 = v_pressure_s32 >> BMP180_TWO_EXPONENT_8;
			v_x1_s32 *= v_x1_s32;
			v_x1_s32 = (v_x1_s32 * 3038) >> BMP180_TWO_EXPONENT_16;
			v_x2_s32 = (v_pressure_s32 * (-7357)) >> BMP180_TWO_EXPONENT_16;
			v_pressure_s32 += (v_x1_s32 + v_x2_s32 + 3791)>> BMP180_TWO_EXPONENT_4;	//Давление в Па
			if ((v_pressure_s32 - (v_pressure_s32/1000)*1000) >419)
				
//				printf("%d\n",(s16)(v_pressure_s32/1000)+1);
				//printl(v_pressure_s32);
				BMP180_Pressure = BMP180_INVALID_DATA;
			else
				//printl(v_pressure_s32);
//				printf("%d\n",(s16)v_pressure_s32/1000);
				BMP180_Pressure = BMP180_INVALID_DATA;
		}
		else
			BMP180_Pressure = BMP180_INVALID_DATA;
	}
	else
		BMP180_Temper = BMP180_INVALID_DATA;
}

/************************************************************************/
/* Давление прочитано                                                   */
/************************************************************************/
void BMP180_PressureReadOk(void){
	BMP180_Pressure = (u32)i2c_Buffer[0]<<16;
	BMP180_Pressure |= (u32)i2c_Buffer[1]<<8;
	BMP180_Pressure |= (u32)i2c_Buffer[1];
	BMP180_Pressure >>= (8-BMP180_OSS);
	SetTask(BMP180_PressureCalc);									//Стартовать расчет с учетом калибровочных коэффициентов
	i2c_Do &= i2c_Free;												//Шину пока освобождаем
}

/************************************************************************/
/* Старт чтения давления												*/
/************************************************************************/
void BMP180_PressureReadStart(void){
	if (i2c_Do & i2c_Busy){											//Шина I2C занята, попытаемся позже
		SetTask(BMP180_PressureReadStart);
		return;
	}
	BMP180_I2C_Fill(i2c_sawsarp, BMP180_DATA_REG_START, 0, BMP180_PRESS_LEN, BMP180_PressureReadOk);
}

/************************************************************************/
/* Запуск измерения давления выполнен, ждем результата измерения        */
/************************************************************************/
void BMP180_PressureCmdOk(void){
	SetTimerTask(BMP180_PressureReadStart, BMP180_PRESSURE_TIME);
	i2c_Do &= i2c_Free;												//Шину пока освобождаем
}

/************************************************************************/
/* Температура прочитана												*/
/************************************************************************/
void BMP180_TemperReadOk(void){
	BMP180_Temper = (u16)i2c_Buffer[0]<<8;
	BMP180_Temper |= (u16)i2c_Buffer[1];
	BMP180_I2C_Fill(i2c_sawp, BMP180_CONTROL_REG, BMP180_PRESSURE_START, BMP180_CMD_LEN, BMP180_PressureCmdOk); //Старт измерения давления
}

/************************************************************************/
/* Старт чтения значения температуры                                    */
/************************************************************************/
void BMP180_TemperReadStart(void){
	if (i2c_Do & i2c_Busy){											//Шина I2C занята, попытаемся позже
		SetTask(BMP180_TemperReadStart);
		return;
	}
	BMP180_I2C_Fill(i2c_sawsarp, BMP180_DATA_REG_START, 0, BMP180_TEMPER_LEN, BMP180_TemperReadOk);
}

/************************************************************************/
/* Успешная передача команды чтения температуры                         */
/************************************************************************/
void BMP180_TemperCmdOk(void){
	SetTimerTask(BMP180_TemperReadStart, BMP180_TEMPERATURE_TIME);	//Ждем результата измерения
	i2c_Do &= i2c_Free;												//Шину пока освобождаем
}

/************************************************************************/
/* Успешное чтение поправочного коэффициента                             */
/************************************************************************/
void BMP180_CoeffReadOk(void){
	*(BMP180Buf+bmp180_AdrCoffCount()) = i2c_Buffer[0];				//Засунуть в буфер прочитанные значения
	*(BMP180Buf+bmp180_AdrCoffCount()+1) = i2c_Buffer[1];
	BMP180_Flag += 2;												//Следующий коэфф.
	if bmp180_AddrCoffIsEnd(){										//Все коэффициенты прочитаны
		BMP180_I2C_Fill(i2c_sawp, BMP180_CONTROL_REG, BMP180_TEPERATURE_START, BMP180_CMD_LEN, BMP180_TemperCmdOk); //Старт измерения температуры
		return;
	}
	BMP180_I2C_Fill(i2c_sawsarp, (BMP180_COEFF_ADR_START+bmp180_AdrCoffCount()), 0, BMP180_COFF_LEN, BMP180_CoeffReadOk);//Продолжаем чтение коэффициентов
}

/************************************************************************/
/* Старт измерения                                                      */
/************************************************************************/
void BMP180_StartMeasuring(void){
	if (i2c_Do & i2c_Busy){											//Шина I2C занята, попытаемся позже
		SetTimerTask(BMP180_StartMeasuring, BMP180_REPEAT_TIME_MS);
		return;
	}
	BMP180_Flag = 0;												//Сбросить счетчики
	BMP180Buf = malloc(BMP180_COFF_COUNT);							//Захватить буфер для калибровочных коэфф.
	if (BMP180Buf == NULL){											//Нету памяти повторить позже
		SetTimerTask(BMP180_StartMeasuring, BMP180_REPEAT_TIME_MS);
		return;
	}
	BMP180_I2C_Fill(i2c_sawsarp,									//Старт чтения калибровочных коэфф.
		BMP180_COEFF_ADR_START+bmp180_AdrCoffCount(),				//Начальный адрес регистра поправочного коэффициента
		0,															//Команда отсутствует
		BMP180_COFF_LEN,											//Количество принимаемых байт
		BMP180_CoeffReadOk);										//По окончании вызвать эту функцию
}
