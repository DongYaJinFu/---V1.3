#include "stm32f10x.h"

//定时器初始化函数
void Timer_Init(void)
{
    //1. 打开定时器的时钟（给定时器通电）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    //2. 配置定时器基本参数
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    //预分频器：把72MHz变成1MHz（72分频）
    //72MHz / 72 = 1MHz = 每1微秒计数一次
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;  // 72分频
    
    //自动重装载值：计数值达到多少时触发
    //1MHz的频率，计1000次就是1毫秒
    TIM_TimeBaseStructure.TIM_Period = 10000 - 1;   // 1ms中断一次
    
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;    // 时钟分割，一般用0
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    //3. 允许定时器产生中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    //4. 配置中断优先级（告诉CPU这个中断有多重要）
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;      // TIM2的中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 响应优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
    NVIC_Init(&NVIC_InitStructure);
    
    //5. 开启定时器（开始计时）
    TIM_Cmd(TIM2, ENABLE);
}



