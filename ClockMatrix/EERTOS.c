/***********************************************************
	����� RTOS
	����� DI-HALT http://easyelectronics.ru/

	���������:
	���� - EERTOS.h
	��������� ���� �� �������� �� ���������� ������ N-�����������,

	�������������
	InitRTOS();			// �������������� ����
	SetTask(Task);		// ������ ������� ������
	RunRTOS();			// ����� ����. 

	//������
	SetTimerTask(Task2,100); //��������� ������ Task2 � ������� ����� 100 ����������� ��� N - ��� ������ ����� ��������
	SetTask(Task); //����� ��������� ������ � �������

	� ������� main ������ ���� ����� ������ 	
		TaskManager();	// ����� ����������
	��� ����� ����. ���� ����� � ���� ���������� ���������
	(������ ����� ���������� �� �������).

	ver. 1.2 ������� - ��������� TiksCount �������� ���������� ����� ������ ������� � ������� ����� ����
***********************************************************/

#include <stddef.h>
#include "EERTOS.h"

//RTOS Interrupt
/*
ISR(RTOS_ISR) ���������� � 
{
TimerService();	
}
*/

// ������� �����, ��������.
// ��� ������ - ��������� �� �������
volatile static TPTR	TaskQueue[TaskQueueSize+1];			// ������� ����������
volatile static struct 	
						{									
						TPTR GoToTask; 						// ��������� ��������
						u16 Time;							// �������� � ��
						} 
						MainTimer[MainTimerQueueSize+1];	// ������� ��������
						
//long int TiksCount;											//������� ����� ������ �������

// RTOS ����������. ������� ��������
inline void InitRTOS(void)
{
u08	index;

for(index=0;index!=TaskQueueSize+1;index++)	// �� ��� ������� ���������� Idle
	{
	TaskQueue[index] = Idle;
	}


for(index=0;index!=MainTimerQueueSize+1;index++) // �������� ��� �������.
	{
	MainTimer[index].GoToTask = Idle;
	MainTimer[index].Time = 0;
	}
	
//TiksCount = 0;
}


//������ ��������� - ������� ����. 
inline void  Idle(void)
{

}


// ������� ��������� ������ � �������. ������������ �������� - ��������� �� �������
void SetTask(TPTR TS)
{

u08		index = 0;
u08		nointerrupted = 0;

if ((TS == Idle) || (TS == NULL)) return;
if (STATUS_REG & (1<<Interrupt_Flag))  // ���� ���������� ���������, �� ��������� ��.
	{
	Disable_Interrupt;
	nointerrupted = 1;					// � ������ ����, ��� �� �� � ����������. 
	}

while(TaskQueue[index]!=Idle) 			// ����������� ������� ����� �� ������� ��������� ������
	{									// �� ��������� Idle - ����� �������.
	index++;
	if (index == TaskQueueSize+1) 		// ���� ������� ����������� �� ������� �� ������ ��������
		{
		if (nointerrupted)	Enable_Interrupt; 	// ���� �� �� � ����������, �� ��������� ����������
		return;									// ������ ������� ���������� ��� ������ - ������� �����������. ���� �����.
		}
	}
												// ���� ����� ��������� �����, ��
TaskQueue[index] = TS;							// ���������� � ������� ������
if (nointerrupted) Enable_Interrupt;				// � �������� ���������� ���� �� � ����������� ����������.
}


//������� ��������� ������ �� �������. ������������ ��������� - ��������� �� �������, 
// ����� �������� � ����� ���������� �������. ��������� ��� ������.
void SetTimerTask(TPTR TS, u16 NewTime)
{
u08		index=0;
u08		nointerrupted = 0;

if (STATUS_REG & (1<<Interrupt_Flag)) 			// �������� ������� ����������, ���������� ������� ����
	{
	Disable_Interrupt;
	nointerrupted = 1;
	}


for(index=0;index!=MainTimerQueueSize+1;++index)	//����������� ������� ��������
	{
	if(MainTimer[index].GoToTask == TS)				// ���� ��� ���� ������ � ����� �������
		{
		MainTimer[index].Time = NewTime;			// �������������� �� ��������
		if (nointerrupted) 	Enable_Interrupt;		// ��������� ���������� ���� �� ���� ���������.
		return;										// �������. ������ ��� ��� �������� ��������. ���� �����
		}
	}
	

for(index=0;index!=MainTimerQueueSize+1;++index)	// ���� �� ������� ������� ������, �� ���� ����� ������	
	{
	if (MainTimer[index].GoToTask == Idle)		
		{
		MainTimer[index].GoToTask = TS;			// ��������� ���� �������� ������
		MainTimer[index].Time = NewTime;		// � ���� �������� �������
		if (nointerrupted) 	Enable_Interrupt;	// ��������� ����������
		return;									// �����. 
		}
		
	}												// ��� ����� ������� return c ����� ������ - ��� ��������� ��������
}


/*=================================================================================
��������� ����� ��. �������� �� ������� ������ � ���������� �� ����������.
*/

inline void TaskManager(void)
{
u08		index=0;
TPTR	GoToTask = Idle;		// �������������� ����������

Disable_Interrupt;				// ��������� ����������!!!
GoToTask = TaskQueue[0];		// ������� ������ �������� �� �������

if (GoToTask==Idle) 			// ���� ��� �����
	{
	Enable_Interrupt;			// ��������� ����������
	(Idle)(); 					// ��������� �� ��������� ������� �����
	}
else
	{
	for(index=0;index!=TaskQueueSize;index++)	// � ��������� ������ �������� ��� �������
		{
		TaskQueue[index]=TaskQueue[index+1];
		}

	TaskQueue[TaskQueueSize]= Idle;				// � ��������� ������ ������ �������
	Enable_Interrupt;							// ��������� ����������
	(GoToTask)();								// ��������� � ������
	}
}


/*
������ �������� ����. ������ ���������� �� ���������� ��� � 1 �� - ��� � ���� ����������� ������ ������ ������. 
���� ����� ����� ����������� � ����������� �� ������

To DO: �������� � ����������� ��������� ������������ ������� ��������. ����� ����� ����� ��������� �� ����� ������. 
� ����� ������������ ��� ������� ������������ �������. 
� ���� ������ �� ������ �������� �������� ����������. 
*/
inline void TimerService(void)
{
u08 index;

//TiksCount++;											//������� ����� ������ ������� ��� ������ ��������� ����

for(index=0;index!=MainTimerQueueSize+1;index++)		// ����������� ������� ��������
	{
	if(MainTimer[index].GoToTask == Idle) continue;		// ���� ����� �������� - ������� ��������� ��������

	if(MainTimer[index].Time !=1)						// ���� ������ �� ��������, �� ������� ��� ���. 
		{												// To Do: ��������� �� ������, ��� ����� !=1 ��� !=0. 
		MainTimer[index].Time --;						// ��������� ����� � ������ ���� �� �����.
		}
	else
		{
		SetTask(MainTimer[index].GoToTask);				// ��������� �� ����? ������ � ������� ������
		MainTimer[index].GoToTask = Idle;				// � � ������ ����� �������
		}
	}
}
