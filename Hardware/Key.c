#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0)
	{
		vTaskDelay(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0);
		vTaskDelay(20);
		KeyNum = 1;
	}
	if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0)
	{
		vTaskDelay(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0);
		vTaskDelay(20);
		KeyNum = 2;
	}
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
	{
		vTaskDelay(20);
		while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);
		vTaskDelay(20);
		KeyNum = 3;
	}
	
	return KeyNum;
}

