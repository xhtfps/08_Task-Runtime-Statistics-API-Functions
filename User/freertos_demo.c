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

char task_buf[512];	/* 任务运行时间统计字符串缓冲区 */

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

/* 任务2:实现任务运行时间统计API函数使用 */
void task2(void * pvParameters)
{
	while(1)
	{
		uint8_t key = key_scan(0);
		if(key == KEY0_PRES)
		{
			vTaskGetRunTimeStats(task_buf);
			printf("%s\r\n", task_buf);
		}
		vTaskDelay(10);
	}
}
