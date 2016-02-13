/***********************************************************
	ЯДОРО RTOS
	Автор DI-HALT http://easyelectronics.ru/

	Настройка:
	файл - EERTOS.h
	Настроить один из таймеров на прерывание каждые N-микросекунд,

	Инициализация
	InitRTOS();			// Инициализируем ядро
	SetTask(Task);		// Запуск фоновой задачи
	RunRTOS();			// Старт ядра. 

	//Работа
	SetTimerTask(Task2,100); //Поставить задачу Task2 в очередь через 100 микросекунд или N - как таймер будет настроен
	SetTask(Task); //Сразу поставить задачу в очередь

	в функции main должен быть вызов фунции 	
		TaskManager();	// Вызов диспетчера
	как можно чаще. Этот вызов и есть выполнение программы
	(помимо задач работающих по таймеру).

	ver. 1.2 удалено - Переменая TiksCount содержит количество тиков службы таймера с момента пуска ядра
***********************************************************/

#include <stddef.h>
#include "EERTOS.h"

//RTOS Interrupt
/*
ISR(RTOS_ISR) Перенесено в 
{
TimerService();	
}
*/

// Очереди задач, таймеров.
// Тип данных - указатель на функцию
volatile static TPTR	TaskQueue[TaskQueueSize+1];			// очередь указателей
volatile static struct 	
						{									
						TPTR GoToTask; 						// Указатель перехода
						u16 Time;							// Выдержка в мс
						} 
						MainTimer[MainTimerQueueSize+1];	// Очередь таймеров
						
//long int TiksCount;											//счетчик тиков службы таймера

// RTOS Подготовка. Очистка очередей
inline void InitRTOS(void)
{
u08	index;

for(index=0;index!=TaskQueueSize+1;index++)	// Во все позиции записываем Idle
	{
	TaskQueue[index] = Idle;
	}


for(index=0;index!=MainTimerQueueSize+1;index++) // Обнуляем все таймеры.
	{
	MainTimer[index].GoToTask = Idle;
	MainTimer[index].Time = 0;
	}
	
//TiksCount = 0;
}


//Пустая процедура - простой ядра. 
inline void  Idle(void)
{

}


// Функция установки задачи в очередь. Передаваемый параметр - указатель на функцию
void SetTask(TPTR TS)
{

u08		index = 0;
u08		nointerrupted = 0;

if ((TS == Idle) || (TS == NULL)) return;
if (STATUS_REG & (1<<Interrupt_Flag))  // Если прерывания разрешены, то запрещаем их.
	{
	Disable_Interrupt;
	nointerrupted = 1;					// И ставим флаг, что мы не в прерывании. 
	}

while(TaskQueue[index]!=Idle) 			// Прочесываем очередь задач на предмет свободной ячейки
	{									// со значением Idle - конец очереди.
	index++;
	if (index == TaskQueueSize+1) 		// Если очередь переполнена то выходим не солоно хлебавши
		{
		if (nointerrupted)	Enable_Interrupt; 	// Если мы не в прерывании, то разрешаем прерывания
		return;									// Раньше функция возвращала код ошибки - очередь переполнена. Пока убрал.
		}
	}
												// Если нашли свободное место, то
TaskQueue[index] = TS;							// Записываем в очередь задачу
if (nointerrupted) Enable_Interrupt;				// И включаем прерывания если не в обработчике прерывания.
}


//Функция установки задачи по таймеру. Передаваемые параметры - указатель на функцию, 
// Время выдержки в тиках системного таймера. Возвращет код ошибки.
void SetTimerTask(TPTR TS, u16 NewTime)
{
u08		index=0;
u08		nointerrupted = 0;

if (STATUS_REG & (1<<Interrupt_Flag)) 			// Проверка запрета прерывания, аналогично функции выше
	{
	Disable_Interrupt;
	nointerrupted = 1;
	}


for(index=0;index!=MainTimerQueueSize+1;++index)	//Прочесываем очередь таймеров
	{
	if(MainTimer[index].GoToTask == TS)				// Если уже есть запись с таким адресом
		{
		MainTimer[index].Time = NewTime;			// Перезаписываем ей выдержку
		if (nointerrupted) 	Enable_Interrupt;		// Разрешаем прерывания если не были запрещены.
		return;										// Выходим. Раньше был код успешной операции. Пока убрал
		}
	}
	

for(index=0;index!=MainTimerQueueSize+1;++index)	// Если не находим похожий таймер, то ищем любой пустой	
	{
	if (MainTimer[index].GoToTask == Idle)		
		{
		MainTimer[index].GoToTask = TS;			// Заполняем поле перехода задачи
		MainTimer[index].Time = NewTime;		// И поле выдержки времени
		if (nointerrupted) 	Enable_Interrupt;	// Разрешаем прерывания
		return;									// Выход. 
		}
		
	}												// тут можно сделать return c кодом ошибки - нет свободных таймеров
}


/*=================================================================================
Диспетчер задач ОС. Выбирает из очереди задачи и отправляет на выполнение.
*/

inline void TaskManager(void)
{
u08		index=0;
TPTR	GoToTask = Idle;		// Инициализируем переменные

Disable_Interrupt;				// Запрещаем прерывания!!!
GoToTask = TaskQueue[0];		// Хватаем первое значение из очереди

if (GoToTask==Idle) 			// Если там пусто
	{
	Enable_Interrupt;			// Разрешаем прерывания
	(Idle)(); 					// Переходим на обработку пустого цикла
	}
else
	{
	for(index=0;index!=TaskQueueSize;index++)	// В противном случае сдвигаем всю очередь
		{
		TaskQueue[index]=TaskQueue[index+1];
		}

	TaskQueue[TaskQueueSize]= Idle;				// В последнюю запись пихаем затычку
	Enable_Interrupt;							// Разрешаем прерывания
	(GoToTask)();								// Переходим к задаче
	}
}


/*
Служба таймеров ядра. Должна вызываться из прерывания раз в 1 мс - это и есть минимальный период вызова задачи. 
Хотя время можно варьировать в зависимости от задачи

To DO: Привести к возможности загружать произвольную очередь таймеров. Тогда можно будет создавать их целую прорву. 
А также использовать эту функцию произвольным образом. 
В этом случае не забыть добавить проверку прерывания. 
*/
inline void TimerService(void)
{
u08 index;

//TiksCount++;											//Счетчик тиков службы таймера для всяких служебных нужд

for(index=0;index!=MainTimerQueueSize+1;index++)		// Прочесываем очередь таймеров
	{
	if(MainTimer[index].GoToTask == Idle) continue;		// Если нашли пустышку - щелкаем следующую итерацию

	if(MainTimer[index].Time !=1)						// Если таймер не выщелкал, то щелкаем еще раз. 
		{												// To Do: Вычислить по тактам, что лучше !=1 или !=0. 
		MainTimer[index].Time --;						// Уменьшаем число в ячейке если не конец.
		}
	else
		{
		SetTask(MainTimer[index].GoToTask);				// Дощелкали до нуля? Пихаем в очередь задачу
		MainTimer[index].GoToTask = Idle;				// А в ячейку пишем затычку
		}
	}
}
