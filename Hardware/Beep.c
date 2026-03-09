#include "stm32f10x.h"

extern uint8_t  Beep_Active;      // 蜂鸣器是否在工作
extern uint16_t Beep_TimeCount;   // 计时
extern uint8_t  Beep_State;       // 当前开关状态

void Beep_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET);
}

void Beep_On(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_8);
}

void Beep_Off(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
}

void Start_Beep(void)
{
    Beep_Active = 1;    // 开启蜂鸣器
    Beep_TimeCount = 0; // 重置计时
}
