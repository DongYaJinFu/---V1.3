#include "stm32f10x.h"                  // Device header
#include "Delay.h"

//外设的头文件
#include "LED.h"
#include "key.h"
#include "OLED.h"
#include "NRF24L01.h"
#include "BEEP.h"
#include "serial.h"

//FreeRTOS的头文件
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define ALARM_LOW  20      	    //滴速过低报警
#define ALARM_HIGH 120     	    //滴速过高报警

uint8_t  g_Drops_per_minute;      //滴速(每分钟多少滴)
uint8_t  g_Bed_Num;		        //病床号
uint8_t  Key_Num;		        //按键
extern uint8_t  Beep_Active;    //蜂鸣器是否在工作
extern uint16_t Beep_TimeCount; //计时
extern uint8_t  Beep_State;     //当前开关状态

//start_task的任务配置
#define START_TASK_PRIO           	   1
#define START_TASK_STACK_SIZE     	   128
TaskHandle_t start_task_handler;
void start_task(void *pvParameters);

//NRF24L01的任务配置
#define NRF24L01_TASK_PRIO             2
#define NRF24L01_TASK_STACK_SIZE       256
TaskHandle_t NRF24L01_TASK_handler;
void NRF24L01_TASK(void *pvParameters);

//OLED的任务配置
#define OLED_TASK_PRIO                 1
#define OLED_TASK_STACK_SIZE           256
TaskHandle_t OLED_TASK_handler;
void OLED_TASK(void *pvParameters);

//BEEP的任务配置
#define BEEP_TASK_PRIO                 1
#define BEEP_TASK_STACK_SIZE           128
TaskHandle_t BEEP_TASK_handler;
void BEEP_TASK(void *pvParameters);

//监控的任务配置
#define MONITOR_TASK_PRIO              1
#define MONITOR_TASK_STACK_SIZE        128
TaskHandle_t MONITOR_TASK_handler;
void MONITOR_TASK(void *pvParameters);

//串口的任务配置
#define SERIAL_TASK_PRIO               1
#define SERIAL_TASK_STACK_SIZE         128
TaskHandle_t SERIAL_TASK_handler;
void SERIAL_TASK(void *pvParameters);

typedef struct 
{
	uint8_t Drops_per_minute;  //滴速(每分钟多少滴)
	uint8_t Bed_Num;		   //病床号
	uint8_t Key_Num;		   //按键
	uint8_t Alarm_type;		   //滴速警告类型, 0:正常, 1:过低, 2:过高
}PatientData_t;

//创建队列的句柄
QueueHandle_t Patient_data_queue;
QueueHandle_t Beep_control_queue;

void freertos_demo(void)
{
	Patient_data_queue = xQueueCreate(5, sizeof(PatientData_t));
	Beep_control_queue = xQueueCreate(3, sizeof(uint8_t));
	
	if(Patient_data_queue == NULL || 
	   Beep_control_queue == NULL)
	{
		OLED_Printf(0, 0, OLED_8X16, "Create queue error!");
	}
	
    xTaskCreate((TaskFunction_t)    start_task,
                (char*)             "start_task",
                (uint16_t)          START_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       START_TASK_PRIO,
                (TaskHandle_t*)     &start_task_handler);
    vTaskStartScheduler();  		//开启任务调度器
}

/*
 * @brief  开启任务, 用于创建所有用到的任务, 该任务创建所有任务之后就自动删除自己
 * @param  无
 * @retval 无
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           
    								
    xTaskCreate((TaskFunction_t)    NRF24L01_TASK,
                (char*)             "NRF24L01_TASK",
                (uint16_t)          NRF24L01_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       NRF24L01_TASK_PRIO,
                (TaskHandle_t*)     &NRF24L01_TASK_handler);

     xTaskCreate((TaskFunction_t)   OLED_TASK,
                (char*)             "OLED_TASK",
                (uint16_t)          OLED_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       OLED_TASK_PRIO,
                (TaskHandle_t*)     &OLED_TASK_handler);      

     xTaskCreate((TaskFunction_t)   BEEP_TASK,
                (char*)             "BEEP_TASK",
                (uint16_t)          BEEP_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       BEEP_TASK_PRIO,
                (TaskHandle_t*)     &BEEP_TASK_handler);  

     xTaskCreate((TaskFunction_t)   MONITOR_TASK,
                (char*)             "MONITOR_TASK",
                (uint16_t)          MONITOR_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       MONITOR_TASK_PRIO,
                (TaskHandle_t*)     &MONITOR_TASK_handler);  	

     xTaskCreate((TaskFunction_t)   SERIAL_TASK,
                (char*)             "SERIAL_TASK",
                (uint16_t)          SERIAL_TASK_STACK_SIZE,
                (void*)             NULL,
                (UBaseType_t)       SERIAL_TASK_PRIO,
                (TaskHandle_t*)     &SERIAL_TASK_handler);				
				
    vTaskDelete(start_task_handler);	
    taskEXIT_CRITICAL();            
}

/*
 * @brief  无线通信的任务, 负责接收另一台机子的消息
 * @param  无
 * @retval 无
 */
void NRF24L01_TASK(void *pvParameters)
{
	PatientData_t patient_data;
	while(1)
	{
		if(NRF24L01_Receive() == 1)		
		{
			patient_data.Bed_Num = 			NRF24L01_RxPacket[0]; //病床号
			patient_data.Drops_per_minute = NRF24L01_RxPacket[1]; //滴速
			patient_data.Key_Num = 			NRF24L01_RxPacket[2]; //按键检测
			
			//滴速阈值判定
			if(patient_data.Drops_per_minute > ALARM_HIGH) 
			{
                patient_data.Alarm_type = 2;
            } 
			else if(patient_data.Drops_per_minute < ALARM_LOW) 
			{
                patient_data.Alarm_type = 1;
            } 
			else 
			{
                patient_data.Alarm_type = 0;
            }
			
			//将结构体patient_data里的数据写到名字为Patient_data_queue的队列里, 并等待时长为:portMAX_DELAY
			if(xQueueSend(Patient_data_queue, &patient_data, portMAX_DELAY) != pdTRUE)
			{
				//创建一个大小为PatientData_t的临时结构体, 当队列满的时候丢弃旧的数据
				PatientData_t temp;
				OLED_Printf(0, 17, OLED_8X16, "Send data error!");
				
				//将旧的数据放进临时的结构体里, 重新发送新的数据
                xQueueReceive(Patient_data_queue, &temp, 0);
                xQueueSend(Patient_data_queue, &patient_data, 0);
			}
			
			//如果收到按键，发送蜂鸣器控制信号
            if(patient_data.Key_Num == 1) 
			{
                uint8_t beep_cmd = 1;
                xQueueSend(Beep_control_queue, &beep_cmd, 0);
			}
		}
		//转换成系统节拍数, 避免移植之后系统节拍为100hz, 单写vTaskDelay(50)的话, 实际上是500ms
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*
 * @brief  OLED任务, 负责打印消息
 * @param  无
 * @retval 无
 */
void OLED_TASK(void *pvParameters)
{
    uint32_t last_update_time = 0;
	PatientData_t PatientData_Receive;
    const TickType_t UPDATE_INTERVAL = pdMS_TO_TICKS(100);
    
    while(1)
    {
        //只在数据更新时刷新OLED，减少不必要的刷新
        if((xTaskGetTickCount() - last_update_time) >= UPDATE_INTERVAL) 
		{
			if(xQueueReceive(Patient_data_queue, &PatientData_Receive, portMAX_DELAY) == pdTRUE)
			{
				OLED_Clear();
				OLED_Printf(0, 0, OLED_6X8, "Bed num:");
				OLED_Printf(0, 10, OLED_6X8, "Drip Rate:");
				
				OLED_ShowNum(50, 0, PatientData_Receive.Bed_Num, 1, OLED_6X8);
				OLED_ShowNum(62, 10, PatientData_Receive.Drops_per_minute, 3, OLED_6X8);
				
				//显示报警状态
				if(PatientData_Receive.Drops_per_minute > ALARM_HIGH) 
				{
					OLED_Printf(0, 20, OLED_6X8, "ALARM: HIGH");
				} 
				else if(PatientData_Receive.Drops_per_minute < ALARM_LOW) 
				{
					OLED_Printf(0, 20, OLED_6X8, "ALARM: LOW");
				}
            }
            OLED_Update();
            last_update_time = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*
 * @brief  BEEP任务, 当病房按下了呼叫按键, 蜂鸣器就负责响闹
 * @param  无
 * @retval 无
 */
void BEEP_TASK(void *pvParameters)
{
	while(1)
	{
		uint8_t beep_cmd;
		const TickType_t BEEP_INTERVAL = pdMS_TO_TICKS(200);  // 蜂鸣间隔
		const TickType_t BEEP_DURATION = pdMS_TO_TICKS(5000); // 总持续5秒
		
		while(1)
		{
			if(xQueueReceive(Beep_control_queue, &beep_cmd, pdMS_TO_TICKS(100)) == pdTRUE)
			{
				if(beep_cmd == 1) 
				{
					TickType_t start_time = xTaskGetTickCount();
					
					while((xTaskGetTickCount() - start_time) < BEEP_DURATION) 
					{
						// 检查是否有新的停止命令
						if(xQueueReceive(Beep_control_queue, &beep_cmd, 0) == pdTRUE) 
						{
							if(beep_cmd == 0) break;
						}
						
						Beep_On();
						vTaskDelay(BEEP_INTERVAL);
						Beep_Off();
						vTaskDelay(BEEP_INTERVAL);
					}
				}
			}
		}
	}
}

/*
 * @brief  监控任务
 * @param  无
 * @retval 无
 */
void MONITOR_TASK(void *pvParameters)
{
	PatientData_t patient_data;
    uint8_t alarm_count = 0;
    const uint8_t ALARM_THRESHOLD = 3;  // 连续3次报警才触发
    
    while(1)
    {
        if(xQueueReceive(Patient_data_queue, &patient_data, portMAX_DELAY) == pdTRUE)
        {
            // 更新全局变量供显示使用
            g_Bed_Num = patient_data.Bed_Num;
            g_Drops_per_minute = patient_data.Drops_per_minute;
            
            // 报警逻辑处理
            switch(patient_data.Alarm_type) 
			{
                case 1:
                    alarm_count++;
                    if(alarm_count >= ALARM_THRESHOLD) 
					{
                        LED0_ON();
                        LED1_OFF();
                    }
                    break;
                case 2:
                    alarm_count++;
                    if(alarm_count >= ALARM_THRESHOLD) 
					{
                        LED1_ON();
                        LED0_OFF();
                    }
                    break;
                default:
                    alarm_count = 0;
                    LED1_OFF();
                    LED0_OFF();
                    break;
            }
            
            // 如果连续报警，可以考虑记录日志
            if(alarm_count >= ALARM_THRESHOLD && patient_data.Alarm_type != 0) 
			{
                // 可以在这里添加日志记录功能
            }
        }
    }
}
