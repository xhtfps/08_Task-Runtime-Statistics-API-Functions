/**
 ****************************************************************************************************
 * @file        freertos.c
 * @author      ALIENTEK
 * @version     V1.4
 * @date        2026-05-29
 * @brief       FreeRTOS 任务实践：任务状态查询
 * @license     Copyright (c) 2020-2032,  ALIENTEK
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F407 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 淘宝地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "freertos_demo.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"

/*FreeRTOS*********************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"

/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务配置
 * 优先级: 1 堆栈大小:128 任务句柄:start_task_handler
 */
#define START_TASK_PRIO			1 
#define START_TASK_STACK_SIZE 	128
TaskHandle_t	start_task_handler;
void start_task(void * pvParameters);

/* TASK1 任务配置
 * 优先级: 2 堆栈大小:128 任务句柄:task1_handler
 */
#define TASK1_PRIO				2 
#define TASK1_STACK_SIZE 		128
TaskHandle_t	task1_handler;
void task1(void * pvParameters);

/* TASK2 任务配置
 * 优先级: 3 堆栈大小:512 任务句柄:task2_handler
 */
#define TASK2_PRIO				3
#define TASK2_STACK_SIZE 		128
TaskHandle_t	task2_handler;
void task2(void * pvParameters);

char task2_info_buf[128];	/* 用于保存任务状态信息的缓冲区 */
/******************************************************************************************************/

/**
 * @brief       FreeRTOS 任务创建
 * @param       无
 * @retval      无
 */
void freertos_demo(void)
{
	xTaskCreate ((TaskFunction_t		) start_task,
				(char *					) "start_task",
				(configSTACK_DEPTH_TYPE	) START_TASK_STACK_SIZE,
				(void * 				) NULL,
				(UBaseType_t			) START_TASK_PRIO,
				(TaskHandle_t *			) &start_task_handler );
	vTaskStartScheduler();		
}

void start_task(void * pvParameters)
{
	taskENTER_CRITICAL();		/* 进入临界区 */
	xTaskCreate ((TaskFunction_t		) task1,
				(char *					) "task1",
				(configSTACK_DEPTH_TYPE	) TASK1_STACK_SIZE,
				(void * 				) NULL,
				(UBaseType_t			) TASK1_PRIO,
				(TaskHandle_t *			) &task1_handler );
				
	xTaskCreate ((TaskFunction_t		) task2,
				(char *					) "task2",
				(configSTACK_DEPTH_TYPE	) TASK2_STACK_SIZE,
				(void * 				) NULL,
				(UBaseType_t			) TASK2_PRIO,
				(TaskHandle_t *			) &task2_handler );
				
	vTaskDelete(NULL);
	taskEXIT_CRITICAL();		/* 退出临界区 */
}

/* 任务1: 实现LED每500ms闪烁 */
void task1(void * pvParameters)
{
	while(1)
	{
		LED0_TOGGLE();
		vTaskDelay(500);
	}
}

/* 任务2:实现任务状态查询API函数 */
void task2(void * pvParameters)
{	
	UBaseType_t priority_num = 0;
	UBaseType_t task_count = 0;		
	UBaseType_t task_state = 0;
	UBaseType_t stack_water_mark = 0;
	TaskStatus_t *status_array = NULL;
	TaskStatus_t *pxTaskStatusArray= NULL;
	TaskHandle_t Task_Handler = NULL;
	eTaskState task_state_enum = eInvalid;

	/*实验一：查询和设置任务优先级*/
	vTaskPrioritySet(task2_handler, 5);						/* 设置当前任务优先级为5 */
	priority_num = uxTaskPriorityGet(task2_handler);		/* 获得当前任务优先级 */
	printf("task2当前任务优先级为：%ld\r\n",priority_num);

	/*实验二：查询任务状态*/
	task_count = uxTaskGetNumberOfTasks();					/* 获取当前系统中任务的数量 */
	printf("当前系统中任务的数量为：%ld\r\n",task_count);

	/*实验三：查询任务状态信息*/
	status_array = mymalloc(SRAMIN, (task_count * sizeof(TaskStatus_t)));		/* 为保存任务状态信息的数组分配内存空间 */
	task_state = uxTaskGetSystemState(status_array, task_count, NULL);			/* 获取当前系统中任务的状态 */
	printf("当前系统中任务的状态为：%ld\r\n",task_state);

	/* 实验四：查询单个任务状态信息*/
	printf("%-12s %-8s %-8s\r\n", "任务名", "任务优先级", "任务编号");
	for(int i = 0; i < task_count; i++)
	{
		// 左对齐：任务名占12字符，优先级和编号各占8字符，整齐统一
		printf("%-12s %-8lu %-8lu\r\n", 
			status_array[i].pcTaskName, 
			status_array[i].uxCurrentPriority, 
			status_array[i].xTaskNumber);
	}
	
	/* 实验五：查询指定任务状态信息 */
	pxTaskStatusArray = mymalloc(SRAMIN, sizeof(TaskStatus_t));	/* 为保存单个任务状态信息的结构体分配内存空间 */
	vTaskGetInfo(task2_handler, pxTaskStatusArray, pdTRUE, eInvalid);	/* 获取task2任务的状态信息 */
	printf("任务名：%s\r\n任务优先级：%lu\r\n任务编号：%lu\r\n任务状态：%s\r\n", 
		pxTaskStatusArray->pcTaskName, 
		pxTaskStatusArray->uxCurrentPriority, 
		pxTaskStatusArray->xTaskNumber, 
		(pxTaskStatusArray->eCurrentState == eRunning) ? "运行" :
		(pxTaskStatusArray->eCurrentState == eReady) ? "就绪" :
		(pxTaskStatusArray->eCurrentState == eBlocked) ? "阻塞" :
		(pxTaskStatusArray->eCurrentState == eSuspended) ? "挂起" :
		(pxTaskStatusArray->eCurrentState == eDeleted) ? "删除" : "未知");

	/* 实验六：查询指定任务句柄 */
	Task_Handler = xTaskGetHandle("task2");	/* 获取task2任务的句柄 */
	if(Task_Handler != NULL)
	{
		printf("任务句柄为：%#x\r\n", (void *)Task_Handler);
	}
	else
	{
		printf("获取任务句柄失败\r\n");
	}
	printf("任务二句柄为：%#x\r\n", (void *)task2_handler);

	/* 实验八：查询任务状态枚举值 */
	task_state_enum = eTaskGetState(task2_handler);	/* 获取task2任务的状态 */
	printf("任务二当前状态为：%s\r\n", 
		(task_state_enum == eRunning) ? "运行" :
		(task_state_enum == eReady) ? "就绪" :
		(task_state_enum == eBlocked) ? "阻塞" :
		(task_state_enum == eSuspended) ? "挂起" :
		(task_state_enum == eDeleted) ? "删除" : "未知");

	/* 实验九：查询系统中所有任务状态信息 */
	vTaskList(task2_info_buf);	/* 输出系统中所有任务的状态信息 */
	printf("任务列表信息：\r\n%s\r\n", task2_info_buf);

	/* 实验七：查询任务最小剩余栈空间 */
	stack_water_mark = uxTaskGetStackHighWaterMark(task2_handler);	/* 获取task2任务的最小剩余栈空间 */
	printf("任务二最小剩余栈空间为：%lu\r\n", stack_water_mark);
	while(1)
	{	
		vTaskDelay(1000);
	}
}

