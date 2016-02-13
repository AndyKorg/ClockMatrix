/*
 * Аппаратный интерфейс к цифровому регулятору громкости
 */ 

#include "VolumeHAL.h"

#define VolumePortIni		do{ SetBit(VOLUME_DDR_CLK_SI, VOLUME_CLK);\
								SetBit(VOLUME_DDR_CLK_SI, VOLUME_SI);\
								SetBit(VOLUME_DDR_CS_SHDWM, VOLUME_CS);\
								SetBit(VOLUME_DDR_CS_SHDWM, VOLUME_SHDWN);\
								ClearBit(VOLUME_PRT_CLK_SI, VOLUME_CLK);}while(0)

#define clkvolLow			ClearBit(VOLUME_PRT_CLK_SI, VOLUME_CLK)
#define clkvolHight			SetBit(VOLUME_PRT_CLK_SI, VOLUME_CLK)
#define csvolLow			ClearBit(VOLUME_PRT_CS_SHDWM, VOLUME_CS)
#define csvolHight			SetBit(VOLUME_PRT_CS_SHDWM, VOLUME_CS)

//------- Описание команд микросхемы регулятора громкости
#define VOLUME_COM_BIT0		4									//Бит С0 команды
#define VOLUME_COM_BIT1		5									//Бит С1 команды
#define VolWriteCommand		Bit(VOLUME_COM_BIT0)				//Команда записи
#define VOLUME_COM_POT0		0									//Команда для потенциомера 0
#define VOLUME_COM_POT1		1									//Команда для потенциомера 1
#define Vol0Pot				Bit(VOLUME_COM_POT0)				//Адрес потенциомера 0
#define Vol1Pot				Bit(VOLUME_COM_POT1)				//Адрес потенциомера 1

/************************************************************************/
/* Передать байт в регулятор громкости                                  */
/************************************************************************/
void VolumeOutByte(u08 Value){
	u08 i;

	for (i=8; i; i--){
		clkvolLow;
		SetBitVal(VOLUME_PRT_CLK_SI, VOLUME_SI, BitIsSet(Value, i-1)?1:0);
		clkvolHight;
	}
	clkvolLow;
}

/************************************************************************/
/* Запись команды и байта данных в регулятор громкости                  */
/************************************************************************/
void VolumeCommand(const u08 Com, const u08 Dat){
	clkvolLow;
	csvolLow;
	VolumeOutByte(Com);
	VolumeOutByte(Dat);
	csvolHight;
}

/************************************************************************/
/* Инициализация интерфейса и установка уровня громкости на минимум     */
/************************************************************************/
void VolumeIntfIni(void){
	VolumePortIni;												//Настроить порты регулятора громкости
	VolumeOffHard;												//Отключить регулятор звука
}

/************************************************************************/
/* Установить громкость                                                 */	
/************************************************************************/
void VolumeSet(u08 Volume){
	VolumeCommand((VolWriteCommand | Vol0Pot), Volume);
}