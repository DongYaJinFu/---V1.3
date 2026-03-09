#include "stm32f10x.h"                  // Device header

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_SetBits(GPIOB, GPIO_Pin_5);
	GPIO_SetBits(GPIOE, GPIO_Pin_5);
}

void LED1_ON(void)
{
	GPIO_ResetBits(GPIOE, GPIO_Pin_5);
}

void LED1_OFF(void)
{
	GPIO_SetBits(GPIOE, GPIO_Pin_5);
}

void LED1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOE, GPIO_Pin_5) == 0)
	{
		LED1_OFF();
	}
	else
	{
		LED1_ON();
	}
}

void LED0_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}

void LED0_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

void LED0_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_5) == 0)
	{
		LED0_OFF();
	}
	else
	{
		LED0_ON();
	}
}
