#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#if 1
//外设的头文件
#include "LED.h"
#include "key.h"
#include "OLED.h"
#include "NRF24L01.h"
#include "BEEP.h"
#include "serial.h"
#include "W25Q64.h"
#endif

//个人操作系统的头文件
#include "My_FreeRTOS.h"

int main(void)
{
	Key_Init();
	LED_Init();
	OLED_Init();
	Beep_Init();
	NRF24L01_Init();
	Serial_Init();
	W25Q64_GPIO_Init();
	
	freertos_demo();
	while (1)
	{
		
	}
}


