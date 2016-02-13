/*
 * �������� ������� � esp8266
 * ��. ..\Web_base\app\include\customer_uart.h
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include "FIFO.h"
#include "Clock.h"
#include "esp8266hal.h"
#include "bits_macros.h"
#include "EERTOS.h"
#include "Alarm.h"
#include "Sound.h"
#include "sensors.h"
#include "i2c.h"
#include "Display.h"
#include "eeprom.h"

#ifdef DEBUG
	#include "Display.h"
#endif

#ifdef ATMEGA644
	#ifdef DEBUG
		#include "usartDebug.h"
	#endif
#endif

#ifdef ATMEGA644
	#define ESP_UART_DATA	UDR1								//������� ������ ��� esp
	#define ESP_UCSRA		UCSR1A
	#define ESP_UCSRB		UCSR1B
	#define ESP_TXC			TXC1								//����� �������� ����
	#define ESP_RXC			RXC1								//������ ����
	#define ESP_UDRE		UDRE1
	#define ESP_UDRIE		UDRIE1								//��������� ���������� �� ��������� ��������
	#define ESP_RXCIE		RXCIE1								//��������� ���������� �� ��������� ������ �����
	#define ESP_UARTRX_vect	USART1_RX_vect						//���������� �� ������ ����� �� esp
	#define ESP_UARTTX_vect USART1_UDRE_vect					//���������� �� ��������� �������� ����� � esp
#else
	#define ESP_UART_DATA	UDR									//������� ������ ��� esp
	#define ESP_UCSRA		UCSRA
	#define ESP_UCSRB		UCSRB
	#define ESP_TXC			TXC									//����� �������� ����
	#define ESP_RXC			RXC									//������ ����
	#define ESP_UDRE		UDRE
	#define ESP_UDRIE		UDRIE								//��������� ���������� �� ��������� ��������
	#define ESP_RXCIE		RXCIE								//��������� ���������� �� ��������� ������ �����
	#define ESP_UARTRX_vect	USART_RXC_vect						//���������� �� ������ ����� �� esp
	#define ESP_UARTTX_vect USART_UDRE_vect						//���������� �� ��������� �������� ����� � esp
#endif

#define espSendRdy()	BitIsSet(ESP_UCSRA, ESP_TXC)			//���������� � esp ����� ������� ������
#define espSendWhite()	while(BitIsClear(ESP_UCSRA, ESP_TXC))	//�������� ��������� �������� �  esp
#define espRxWhile()	while(BitIsClear(ESP_UCSRA, ESP_RXC))	//�������� ������ ����� �� esp
#define espRxStart()	SetBit(ESP_UCSRB, ESP_RXCIE)			//��������� ���������� ��� ������ �����
#define espRxStop()		ClearBit(ESP_UCSRB, ESP_RXCIE)
#define epsTxStart()	SetBit(ESP_UCSRB, ESP_UDRIE)			//��������� ���������� �� ���������� �����������
#define espTxStop()		ClearBit(ESP_UCSRB, ESP_UDRIE)			//���������

#define ESP_RESET_PORT	PORTA									//���� ���� ������ esp
#define ESP_RESET_DDR	DDRA
#define ESP_RESET_BIT	PORTA3
#define espResetInit()	do{SetBit(ESP_RESET_DDR, ESP_RESET_BIT); SetBit(ESP_RESET_PORT, ESP_RESET_BIT);}while(0)
#define espReset()		ClearBit(ESP_RESET_PORT, ESP_RESET_BIT)
#define espStart()		SetBit(ESP_RESET_PORT, ESP_RESET_BIT)

#define ESP_ERR_TIMEOUT	2000										//������������ ����� �������� ������ �� ������, ��

#define CMD_CLCK_PILOT1		0xaa								//������ ���� ������ �������
#define CMD_CLCK_PILOT2		0x55								//������ ���� ������ �������
#define CLK_CMD_PILOT_NUM	4									//����� ������ ��� �������

#define CLK_CMD_EMPTY	0x00									//����������� ������ �������, ��� ����� ������� ����
#define CLK_TIMEOUT		3000									//������� �������� ���������� ������ �� ����� ��

#define CLK_MAX_LEN_CMD	32										//������������ ����� ������� uart. ������ ���� �������� ������ ��. fifo.h

//����� FIFO ������ ��� ������ � ��������
FIFO(CLK_MAX_LEN_CMD)
	espTxBuf, espRxBuf;

//############################ ������� � ���������� UART  ###########################################################################################
typedef void (*VOID_CLK_UART_CMD)(u08 cmd, u08* pval, u08 valLen);//���� ������� � � ������

typedef const PROGMEM struct{									//������� UART
	u08 CmdCode;
	VOID_CLK_UART_CMD Func;										//���� Value �� NULL, �� ������� �� ����������
} PROGMEM pEspUartCmd;

//������� ����������� �� ������ esp
void UartWatchSet(u08 cmd, u08* pval, u08 valLen);				//�������� ����� �� esp
void UartAlarmSet(u08 cmd, u08* pval, u08 valLen);				//��������� �������� �����������
void UartVolumeSet(u08 cmd, u08* pval, u08 valLen);				//���������
void UartSensorSet(u08 cmd, u08* pval, u08 valLen);				//���������� ���������
void UartFontSet(u08 cmd, u08* pval, u08 valLen);				//���������� �����
void UartGetAll(u08 cmd, u08* pval, u08 valLen);				//��������� ��� ��������� �����
void UartBaseModeSet(u08 cmd, u08* pval, u08 valLen);			//������� � �������� ����� ��� ��������� ������� CLC_STOP
void UartStationIP(u08 cmd, u08* pval, u08 valLen);				//������� IP ����� station
void UartHorizontalSpeed(u08 cmd, u08* pval, u08 valLen);		//���������� �������� ������� ������

//-------------------- ������ ������ � ���������� ��� ������ �� UART ---------------------------------------------------
#define UART_CMD_NUM_MAX 9										//���������� ������ uart
const pEspUartCmd PROGMEM espUartCmd[UART_CMD_NUM_MAX] = {
	/*	CmdCode, 		Func */
	{CLK_WATCH,			UartWatchSet},
	{CLK_ALARM, 		UartAlarmSet},
	{CLK_VOLUME,		UartVolumeSet},
	{CLK_SENS,			UartSensorSet},
	{CLK_FONT,			UartFontSet},
	{CLK_STOP,			UartBaseModeSet},
	{CLK_ALL,			UartGetAll},
	{CLK_ST_WIFI_IP,	UartStationIP},
	{CLK_HZ_SPEED, 		UartHorizontalSpeed},
		
/*
--#define CLK_NTP_START	0x03	//��������� ��������� ������� �� ntp �������
#define CLK_STOP		0x08	//���������� �������� (������������ ��� ��������)

//������
#define CLK_NTP_ERROR	0x70	//������ ��������� � ������� ntp
*/	
};
#define espCmdCode(id)			(pgm_read_byte(&espUartCmd[id].CmdCode))
#define espCmdExec(id)			*((VOID_CLK_UART_CMD*)pgm_read_word(&espUartCmd[id].Func))

volatile u08  espStatusFlags;									//��������� ������ - ���� �������, ������� ������� ����� � �������
//								7								
//								6
//								5	��������� ����
//								4
#define ESP_ATTEMPT_MASK		0b00001111						//����� �������� ������� ����� � �������

#define espAttemptLinkSet()		do{espStatusFlags |= ESP_ATTEMPT_MASK;}while(0)
#define espAttemptIsEnded()		((espStatusFlags & ESP_ATTEMPT_MASK)==0)	//������� ���������
#define espTimeoutGoing()		(espStatusFlags & ESP_ATTEMPT_MASK)	//�������� ������ ��������

void espTimeoutError(void);										//�������� ������� ������ ������

u08* espStationIP = NULL;										//����� IP � ���� ������ XXX.XXX.XXX.XXX

/************************************************************************/
/* �������� ������ � esp                                                */
/************************************************************************/
void espUartTx(u08 Cmd, u08* Value, u08 Len){
	if espModuleIsNot()											//��� ������, ������ �� ������
		return;
	while(FIFO_SPACE(espTxBuf) < (Len+6)){						//���� ���� ����� �� �����������
		TaskManager();
		if espModuleIsNot(){									//�� ������� ���������� ������� ������
			return;
		}
	}

	u08 FlagInt = SREG & SREG_I;
	cli();														//����������� �����
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT1);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT2);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT1);
	FIFO_PUSH(espTxBuf, CMD_CLCK_PILOT2);
	FIFO_PUSH(espTxBuf, Len);
	while(Len){
		FIFO_PUSH(espTxBuf, *Value++);
		Len--;
	}
	FIFO_PUSH(espTxBuf, Cmd);
	epsTxStart();
	if (FlagInt)
		sei();
	espAttemptLinkSet();
	SetTimerTask(espTimeoutError, ESP_ERR_TIMEOUT);				//������� ������ �� ������
}

#ifdef DEBUG
void UartDebugOut(u08 *ptr, u08 Len){
	if (Len < CLK_MAX_LEN_CMD)
		espUartTx(ClkWrite(CLK_DEBUG), ptr, Len);
	else{
		u08 err[] = "buf owerflow";
		espUartTx(ClkWrite(CLK_DEBUG), err, sizeof(err));
	}
}
#endif

/************************************************************************/
/* ��������� ��������� ���������� ������                                */
/************************************************************************/
void espVarInit(void){
	u08 i;
	
	espNetNameSet();											//������������ � ���� wifi ��� station ���� ���� ��� ���� � ������ �� sd-�����
	espWatchTx();												//�����
	for (i=0;i<ALARM_MAX;i++){									//����������
		espUartTx(ClkWrite(CLK_ALARM), (u08*) ElementAlarm(i), sizeof(struct sAlarm));
	}
	espVolumeTx(vtAlarm);										//��������� � �������
	espVolumeTx(vtButton);
	espVolumeTx(vtEachHour);
	for(i=0;i<SENSOR_MAX;i++)									//�������
		espSendSensor(i);
	i = FontIdGet();
	espUartTx(ClkWrite(CLK_FONT), &i, 1);						//�����
	if (RTC_IsNotValid())										//��� ���� �������, ��������� ������ ������� �� ���������
		StartGetTimeInternet();

	u08 HZConst[HORZ_SCROLL_CMD_COUNT];							//�������� ������� ������ - ����� 2 �����, ���� ��� �������� ���������� �����, ���� ��� �������� �������� ��������				
	HZConst[0] = HORZ_SCROLL_STEPS;
	HZConst[1] = HorizontalSpeed/HORZ_SCROOL_STEP;
	espUartTx(ClkWrite(CLK_HZ_SPEED), HZConst, HORZ_SCROLL_CMD_COUNT);
}

/************************************************************************/
/* ������ ���� ������													*/
/************************************************************************/
void espCheckStart(void){
	u08 i;
	espUartTx(ClkTest(CLK_CMD_EMPTY), &i, 1);					//���������� �������� �������
}

/************************************************************************/
/* ������� ������ �� ������												*/
/************************************************************************/
void espTimeoutError(void){
	if (espAttemptIsEnded()){								//�������� �������?
		espModuleRemove();									//�������, ��� ������ �����������
	}
	else{
		espStatusFlags--;									//����� ���� �������
		espReset();											//����������
		FIFO_FLUSH(espTxBuf);
		//SetTimerTask(espCheckStart, CLK_TIMEOUT);			//� ��������� �������� ������
		espStart();
		SetTimerTask(espTimeoutError, ESP_WHITE_START);		//���� ���� ������ �� ������������
	}
}

/************************************************************************/
/* ����� � ���������� ������� � �������� ������							*/
/************************************************************************/
void espParse(void){
	u08 Len = FIFO_FRONT(espRxBuf);
	u08* ptrData = malloc(Len);
	
	if (ptrData == NULL)										//�� ������� ������ ��� ��������� �������, �������� ����� ��������
		SetTask(espParse);
	else{
		u08* Data = ptrData;
		FIFO_POP(espRxBuf);
		for(u08 i=Len;i;i--){									//��������� ������ �� ������
			*Data = FIFO_FRONT(espRxBuf);
			Data++;
			FIFO_POP(espRxBuf);
		}
		u08 Cmd = FIFO_FRONT(espRxBuf);
		FIFO_POP(espRxBuf);
		for (u08 i=0; i < UART_CMD_NUM_MAX; i++) {
			if (espCmdCode(i) == ClkCmdCode(Cmd)){				//������� �������
				((VOID_CLK_UART_CMD)&espCmdExec(i))(Cmd, ptrData, Len);//������������
				break;
			}
		}
		free(ptrData);
	}
}
/************************************************************************/
/* ������ ���� �� esp                                                   */
/************************************************************************/
ISR(ESP_UARTRX_vect){
	u08 rxbyte = ESP_UART_DATA;
	static u08 CountByte = 0, CountData = 0;
	
	#define CountByteVal()		(CountByte & 0x3f)
	#define SetCustomTxtCmd()	SetBit(CountByte, 7)			//����������� ������� ��� ������������� ������
	#define CustomTxtCmdIsSet()	BitIsSet(CountByte, 7)
	#define CustomTxtEnd()		SetBit(CountByte, 6)			//��� ����� ������������ ������
	#define CustomTxtIsNoEnd()	BitIsClear(CountByte, 6)			
	
	if (CountByteVal() < CLK_CMD_PILOT_NUM){					//��������� �����-�����
		if (rxbyte == ((CountByte & 1)?CMD_CLCK_PILOT2:CMD_CLCK_PILOT1)) //���� ��������������� ���� ������
			CountByte++;
		else
			CountByte = 0;
	}
	else{														//���� �������������
		espModuleSet();											//���� ������� ������ ����������
		espAttemptLinkSet();									//������������ ���������� ������� ����� � �������
		if (CountByteVal() == CLK_CMD_PILOT_NUM){				//������� ����� ������
			CountData = rxbyte;
			CountByte++;
			if (rxbyte > CLK_MAX_LEN_CMD){						//���� ���������� ������ ������ ������������ ����� �������, �� ��� ����������� ������� CLK_CUSTOM_TXT
				SetCustomTxtCmd();
				CountData++;									//�������� ������� ��� �� ��� ������� �� �� ������� ��� ������
				ClearDisplay();									//����������� �������
				CreepOn(1);
			}
			else{
				if ((rxbyte+2)>FIFO_SPACE(espRxBuf)){			//��� �������� ����� � ������ ������� ������� ������������
					CountByte = 0;								//����� 2 ��� ���� ����� � ���� �������
				}
				else{											//���� ��������� ����� � ������, ���� ���� ����� ������
					FIFO_PUSH(espRxBuf, rxbyte);
				}
			}
		}
		else{
			if CustomTxtCmdIsSet(){								//����������� ������� ������������� ������, ������� ����� � ����� �������
				if ((CountData != 1) && CustomTxtIsNoEnd()){	//��������� ���� ��� ��� �������, ��� ���� ������������ � ����� ������ ��� �� ����
					if (rxbyte){								//������� ���� ��� ����� ������, ����� ���� ������ �� ���������
						if (rxbyte == '_')						//���� ������������� �������������� � ������ ��-�� ���� � �������� esp8266
							sputc(S_SPACE, UNDEF_POS);
						else
							sputc(rxbyte, UNDEF_POS);
						sputc(S_SPICA, UNDEF_POS);
					}
					else{
						sputc(S_SPICA, UNDEF_POS);
						CustomTxtEnd();
					}
				}
			}
			else{
				FIFO_PUSH(espRxBuf, rxbyte);					//����������� ����� ������ � ��� �������
			}
			if (CountData){
				CountData--;
			}
			else{												//��� �������
				if (CustomTxtCmdIsSet())						//���� ����������� ������� ������������� ������, �� ��������� ����� ����������� � �������
					ShowDate();
				else
					SetTask(espParse);
				CountByte = 0;									//��������� ���� �� �����
			}
		}
	}
}

/************************************************************************/
/* ���������� ����, ����� ������ ����������                             */
/************************************************************************/
ISR(ESP_UARTTX_vect){
	if( FIFO_IS_EMPTY(espTxBuf) ) {								//���� ������ � fifo ������ ��� �� ��������� ��� ����������
		espTxStop();
	}
	else {														//����� �������� ��������� ����
		u08 txbyte = FIFO_FRONT(espTxBuf);
		FIFO_POP(espTxBuf);
		ESP_UART_DATA = txbyte;
	}
}

/************************************************************************/
/* ������������� USART. ���� IntOn - 1 �� ���������� ����������         */
/************************************************************************/
#define ESP_SERIAL_INT_ON	1
#define ESP_SERIAL_INT_OFF	0
inline void espSerialIni(u08 IntOn){

#ifdef ATMEGA644
	//-------- ��������� ����� ��� ������
	#undef BAUD											//��� ��������� �������� ��� ������
//	#define BAUD 9600
	#define BAUD 115200
	#include <util/setbaud.h>
	UCSR1A = 0x00;										//��� �� ���������
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) |					//��������� ����� � ��������
				((IntOn?1:0)<<RXCIE1);					//���������� �� ��������� �������� ��� �������������
	UCSR1C = (0<<UMSEL11) | (0<<UMSEL10) |				//Asynchronius USART
			(0<<UPM11) | (0<<UPM10) |					//Parity off
			(0<<USBS1) |								//Stop bit = 1
			(0<<UCSZ12) | (1<<UCSZ11) | (1<<UCSZ10) |	//8 ��� ������
			(0<<UCPOL1);								//Polary XCK �� ����� �.�. �� ������������ � ���������� ������
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	#if USE_2X											//���� �������� �� ������ ������������ ���������� �� 2
		UCSR1A |= (1 << U2X1);
	#else
		UCSR1A &= ~(1 << U2X1);
	#endif
#else
	//-------- ��������� ����� ��� ������
	#undef BAUD											//��� ��������� �������� ��� ������
//	#define BAUD 115200
	#define BAUD 19200
	#include <util/setbaud.h>
	ESP_UCSRA = 0x00;									//��� �� ���������
	ESP_UCSRB = (1<<RXEN) | (1<<TXEN) |					//��������� ����� � ��������
				((IntOn?1:0)<<RXCIE);					//���������� �� ��������� �������� ��� �������������
	UCSRC =  (1<<URSEL) |								//������ � UCSRC
			 (0<<UMSEL) |								//Asynchronius USART
			 (0<<UPM1) | (0<<UPM0) |					//Parity off
			 (0<<USBS) |								//Stop bit = 1
			 (0<<UCSZ2) | (1<<UCSZ1) | (1<<UCSZ0) |		//8 ��� ������
			 (0<<UCPOL);								//Polary XCK �� ����� �.�. �� ������������ � ���������� ������
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X											//���� �������� �� ������ ������������ ���������� �� 2
		UCSRA |= (1 << U2X);
	#else
		UCSRA &= ~(1 << U2X);
	#endif
#endif
FIFO_FLUSH(espTxBuf);
FIFO_FLUSH(espRxBuf);
}

/************************************************************************/
/* ������������� ���������� � ������ �������� �������� ������			*/
/************************************************************************/
void espInit(void){
	espResetInit();										//���� ����� ������ ����������������
	espReset();											//�������� ������
	espSerialIni(ESP_SERIAL_INT_ON);					//���������������� USART
	espAttemptLinkSet();								//���� �������
	espStart();											//������ ��������� - �.�. ���� ����� ������ � ������� ���������
	espModuleRemove();									//������� ��� ������ �����������
	SetTimerTask(espTimeoutError, ESP_WHITE_START);		//���� ������� �� ������ ������
}

/************************************************************************/
/* ���������� 1 ���� ������ ����������                                  */
/* �������� � �������, �.�. ������������ � ����                         */
/************************************************************************/
u08 espInstalled(void){
	return espModuleIsSet()?1:0;
}

/************************  ���������� ������� ***************************/

/************************************************************************/
/* ���������� �������� ������� ������			                        */
/************************************************************************/
void UartHorizontalSpeed(u08 cmd, u08* pval, u08 valLen){
	if ClkIsWrite(cmd){
		if (valLen == HORZ_SCROLL_CMD_COUNT){			//������ ���� ��� �����
			if ((pval[0] == HORZ_SCROLL_STEPS) && (pval[1]<=HORZ_SCROLL_STEPS)){	//� ���������� ���������
				HorizontalSpeed = HORZ_SCROOL_STEP*((u16)pval[1]);
				EeprmStartWrite();						//�������� � eeprom ����� ��������
			}
		}
	}
}

/************************************************************************/
/* ��������� ������� ������� ������� �� ���������                       */
/************************************************************************/
void StartGetTimeInternet(void){
	u08 i;
	espUartTx(ClkWrite(CLK_NTP_START), &i, 1);
}		

/************************************************************************/
/* ������� �� ������ �������� ����������								*/
/************************************************************************/
void UartAlarmSet(u08 cmd, u08* pval, u08 valLen){
	if ClkIsWrite(cmd){
		if (pval[0]<=ALARM_MAX){
			memcpy(ElementAlarm(pval[0]), (void*)pval, valLen);
			EeprmStartWrite();
		}
		EeprmStartWrite();										//�������� � eeprom
	}
}

/************************************************************************/
/* ��������� ��������� � ������			                                */
/************************************************************************/
void espVolumeTx(enum tVolumeType Type){
	struct espVolume Vol;
	
	Vol.id = Type;
	Vol.Volume = VolumeClock[Type].Volume;
	Vol.State = 0;												//��� ����������� ��������� ���������� �� �����
	if (Type == vtButton)
		Vol.State = KeyBeepSettingIsSet();
	else if (Type == vtEachHour)
		Vol.State = EachHourSettingIsSet();
	Vol.LevelMetod = VolumeClock[Type].LevelVol;
	espUartTx(ClkWrite(CLK_VOLUME), (u08*) &Vol, sizeof(Vol));
}

/************************************************************************/
/* ������� ������� ���������� ����������                                */
/************************************************************************/
void UartVolumeSet(u08 cmd, u08* pval, u08 valLen){

	#define espVol	((struct espVolume*)pval)

	if ClkIsWrite(cmd){									//������ ��������
		VolumeClock[espVol->id].Volume = espVol->Volume;
		VolumeClock[espVol->id].LevelVol = espVol->LevelMetod;	//��� ������������� ���������
		if (espVol->id == vtButton){					//���� ������
			KeyBeepSettingSetOff();
			if (espVol->State)
				KeyBeepSettingSwitch();
		}
		else if(espVol->id == vtEachHour){				//�������
			EachHourSettingSetOff();					//������� ���������
			if (espVol->State)
				EachHourSettingSwitch();				//�� ���� ��� ���� ��������
		}
		EeprmStartWrite();								//�������� � eeprom
#ifdef DEBUG
SetTimerTask(espNetNameSet, 10000);
#endif
	}
	else if ClkIsTest(cmd){								//���� ���������
		if (SoundIsBusy()){								//���������� ���� ���� �� ������
			SoundOff();
			return;
		}
		struct sVolume Tmp=VolumeClock[pval[0]];		//��������� ���������� ����������
		u08 State = KeyBeepSettingIsSet()?1:0;
		u08 TestType = SND_TEST;						//�� ��������� ���� ������ �����������
		VolumeClock[espVol->id].LevelVol = vlConst;		//��� ������������� ��� ���� ���������
		VolumeClock[espVol->id].Volume = espVol->Volume;//������� ���������
		if(espVol->id == vtEachHour){					
			TestType = SND_TEST_EACH;
			State = EachHourSettingIsSet()?1:0;
			
		}
		else if (espVol->id == vtAlarm){
			TestType = SND_TEST_ALARM;
			State = 0;
			VolumeClock[espVol->id].LevelVol = espVol->LevelMetod;	//��� ���������� ����
		}
		SoundOn(TestType);								//���� � ��������� ��������� ���������
		VolumeClock[espVol->id] = Tmp;					//���������� ������ ��������
		struct espVolume espRet = *espVol;
		espRet.LevelMetod = Tmp.LevelVol;
		espRet.Volume = Tmp.Volume;
		espRet.State = State;
		espUartTx(ClkWrite(CLK_VOLUME), (u08*)&espRet, sizeof(espVol));
	}
}

/************************************************************************/
/* �������� �������� �������											*/
/************************************************************************/
void espSendSensor(u08 numSensor){
	u08 cmd[sizeof(struct sSensor)+1];
	u08* cmd_ptr = cmd;
	
	memcpy((void*)(cmd_ptr+1), (void *)SensorNum(numSensor), sizeof(struct sSensor));		//����������� ������� ������ ������� ���� ��� ������
	cmd[0] = numSensor;
	espUartTx(ClkWrite(CLK_SENS), cmd, sizeof(cmd));
}

/************************************************************************/
/* ���������� ���������                                                 */
/************************************************************************/
void UartSensorSet(u08 cmd, u08* pval, u08 valLen){	

	if (ClkIsWrite(cmd)){
		if (pval[0]<=SENSOR_MAX){
			u08 ptr[sizeof(struct sSensor)+1];					//������������� �����
			memcpy((void*)ptr, (void*)pval, valLen);			//��������� �����
			u08 *p = ptr;										//��������� �� ������ �������
			p++;
			memcpy(SensorNum(pval[0]), (void*)p, valLen-1);
			EeprmStartWrite();									//�������� � eeprom
		}
	}
	else if (ClkIsTest(cmd)){
		for(u08 i=0; i<=SENSOR_MAX;i++){	
			if (SensorNum(i)->Adr == pval[0]){
				CurrentSensorShow = i;
				SetClockStatus(csSensorSet, ssSensWaite);
				espSendSensor(i);
				break;
			}
		}
	}
}

/************************************************************************/
/* �������� � ������ ������� �����                                      */
/************************************************************************/
void espWatchTx(void){
	espUartTx(ClkWrite(CLK_WATCH), (u08*) &Watch, sizeof(Watch));
}
/************************************************************************/
/* ������� �� ������ �����												*/
/************************************************************************/
void UartWatchSet(u08 cmd, u08* pval, u08 valLen){
	if (ClkIsWrite(cmd)){
		while(i2c_Do & i2c_Busy)						//��������� ������������ ���� I2C
			TaskManager();
		memcpy((void*) &Watch, pval, sizeof(Watch));
		i2c_RTC_DirectWrt();
		if (RTC_IsNotValid())							//��� ���� �������, �� ������ �� ���������
			RTC_ValidSet();								//���� ���������
		if ((ClockStatus == csTune) && (SetStatus == ssTimeSet)){	//���� ����� ������� ������� �� ���������� �� �������� �� ������� ��������� �������
			SetClockStatus(csTune, ssTimeLoaded);
		}
	}
}

/************************************************************************/
/* ���������� ����� � �����                                             */
/************************************************************************/
void UartFontSet(u08 cmd, u08* pval, u08 valLen){
	if (*pval<FONT_COUNT_MAX){
		if (ClkIsWrite(cmd)){
			FontIdSet(*pval);
			SetClockStatus(csClock, ssNone);
			EeprmStartWrite();							//�������� � eeprom
		}
	}
}

/************************************************************************/
/* �������� � ������ ��� ���� � ������ ��� ������ station � SD-�����    */
/************************************************************************/
void espNetNameSet(void){
	#define ESP_NET_VAL "netname="
	#define ESP_PAS_VAL "password="
	
	FRESULT Ret;
	WORD rb;												//����������� ����
	char *NetName, *pass;
	u16 i;

	if NoSDCard() return;									//��� sd ����� - �� � ��� ��������
	if espModuleIsNot() return;								//������ ��� � �������
	
	if (SoundIsBusy()){
		SetTask(espNetNameSet);								//FatFs ������, ���������� �����
		return;
	}	

	if (pf_disk_is_mount() != FR_OK){						//����� ��� �� ������������, ������� �� ����������������.
		Ret = pf_mount(&fs);
		if (Ret != FR_OK) return;
	}
	Ret = pf_open("wifi.cfg");
	if (Ret == FR_OK){
		Ret = pf_lseek (0);
		if (Ret == FR_OK){
			Ret = pf_read(&read_buf0, BUF_LEN, &rb);
			if ((Ret == FR_OK) && (rb>(strlen(ESP_NET_VAL)+strlen(ESP_PAS_VAL)+4))){	//����������� ���� ������ ���� ������ ����� ����������
				*(read_buf0+rb+1) = 0;						//����� ������
				for(i=0;i<BUF_LEN;i++){	//������ �������� ������ � ������ � 0
					if (*(read_buf0+i)<0x20){
						*(read_buf0+i) = 0;
					}
				}
				NetName = (void*)strstr((char*)read_buf0, ESP_NET_VAL);	//��� ����
				if (NetName != NULL){
					NetName += strlen(ESP_NET_VAL);
					for(i=0; *(NetName+strlen(NetName)+i) == 0;i++);	//���������� ���� ����� ��������
					pass = (void*)strstr(NetName+strlen(NetName)+i, ESP_PAS_VAL);//������
					if (pass != NULL){
						pass += strlen(ESP_PAS_VAL);
						memset(read_buf1, 0, strlen(NetName)+strlen(pass)+8);
						memcpy(read_buf1, NetName, strlen(NetName));
						memcpy(read_buf1+strlen(NetName)+1, pass, strlen(pass));
//						espUartTx(ClkWrite(CLK_ST_WIFI_AUTH), read_buf1, strlen(NetName)+strlen(pass)+2);
						espUartTx(ClkRead(CLK_ST_WIFI_IP), read_buf1, strlen(NetName)+strlen(pass)+2);
					}
				}
			}
		}
	}
	pf_mount(0);
}

/************************************************************************/
/* ������ IP ������ ��� ������ station. �������� � ��������� ��� ������ */
/* �� ����																*/
/************************************************************************/
void espGetIPStation(void){
	u08 i;
	espUartTx(ClkRead(CLK_ST_WIFI_IP), &i, 1);
}

/************************************************************************/
/* IP ����� �������                                                     */
/************************************************************************/
void UartStationIP(u08 cmd, u08* pval, u08 valLen){
	#define IP_LEN_STR (3*4+5)						//������ ������ �� ��� ����� ���� ����� � ������� ����

	if (espStationIP == NULL){
		espStationIP = malloc(IP_LEN_STR);
	}
	memcpy(espStationIP, pval, IP_LEN_STR);
	SetClockStatus(csTune, ssIP_Show);				//������� �� �������
}

/************************************************************************/
/* ������� � �������� ����� ��� ��������� ������� CLC_STOP				*/
/************************************************************************/
void UartBaseModeSet(u08 cmd, u08* pval, u08 valLen){
	SetClockStatus(csClock, ssNone);
}

/************************************************************************/
/* ��������� ��� ��������� � ��������  � �����                          */
/************************************************************************/
void UartGetAll(u08 cmd, u08* pval, u08 valLen){
	if ClkIsRead(cmd)
		espVarInit();
}
