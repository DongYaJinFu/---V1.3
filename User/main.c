#include "stm32f10x.h"                  // Device header
#include "Delay.h"

//外设的头文件
#include "LED.h"
#include "key.h"
#include "OLED.h"
#include "NRF24L01.h"
#include "BEEP.h"
#include "serial.h"

//个人操作系统的头文件
#include "My_FreeRTOS.h"
	                          
uint8_t  Beep_Active = 0;      // 蜂鸣器是否在工作
uint16_t Beep_TimeCount = 0;   // 计时
uint8_t  Beep_State = 0;       // 当前开关状态

int main(void)
{
	Key_Init();
	LED_Init();
	OLED_Init();
	Beep_Init();
	NRF24L01_Init();
	Serial_Init();
	
	freertos_demo();
	while (1)
	{
		
	}
}

#if 0
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
      
    }
}
#endif

