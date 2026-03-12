#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "string.h"

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

uint8_t  g_Drops_per_minute;    //滴速(每分钟多少滴)
uint8_t  g_Bed_Num;		        //病床号
uint8_t  Key_Num;		        //按键
extern uint8_t  Beep_Active;    //蜂鸣器是否在工作
extern uint16_t Beep_TimeCount; //计时
extern uint8_t  Beep_State;     //当前开关状态
char Serial_RxPacket_Name[10];  //存放名字
uint8_t Serial_RxPacket_Age[1]; //存放年龄
char Serial_RxPacket_Sex[8];    //存放性别

//定义串口中断函数里的包头字符串
const char *HEADER_NAME = "NAME:";
const char *HEADER_AGE = "AGE:";
const char *HEADER_SEX = "SEX:";

enum
{
	wait_header,
	match_name,
	match_age,
	match_sex,
	receive_data,
	end
};

#if 1
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
#endif

typedef struct 
{
    char    Name[10];          //姓名
    uint8_t Age;	    	   //年龄
    char    Sex[8];    		   //性别
   	uint8_t Bed_Num;		   //病床号
	uint8_t Drops_per_minute;  //滴速(每分钟多少滴)
	uint8_t Key_Num;		   //按键
	uint8_t Alarm_type;		   //滴速警告类型, 0:正常, 1:过低, 2:过高
} PatientData_t;

//创建队列集的句柄
QueueSetHandle_t queueSet_handle;

//创建队列的句柄
QueueHandle_t Patient_data_queue;		//NFR24L01发送的数据
QueueHandle_t Rx_patientdata_queue;		//串口发送的数据
QueueHandle_t Beep_control_queue;		//用于给蜂鸣器任务发出信号
QueueHandle_t Alarm_type_queue;			//用于给监控任务发出信号

void freertos_demo(void)
{
	//创建队列集
	queueSet_handle = xQueueCreateSet(2);

	if(queueSet_handle == NULL)
	{
		OLED_Clear();
		OLED_Printf(0, 0, OLED_8X16, "Create queueset error!");
	}

    //创建队列
	Patient_data_queue   = xQueueCreate(5, sizeof(PatientData_t));
	Rx_patientdata_queue = xQueueCreate(5, sizeof(PatientData_t));
	Beep_control_queue   = xQueueCreate(1, sizeof(uint8_t));
	Alarm_type_queue     = xQueueCreate(1, sizeof(uint8_t));

	if( Patient_data_queue   == NULL || 
		Rx_patientdata_queue == NULL ||
		Beep_control_queue   == NULL ||
		Alarm_type_queue     == NULL)
	{
        OLED_Clear();
		OLED_Printf(0, 0, OLED_8X16, "Create queue error!");
	}

	//把两个队列添加入队列集里
	xQueueAddToSet(Patient_data_queue, queueSet_handle);
	xQueueAddToSet(Rx_patientdata_queue, queueSet_handle);

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
	uint8_t alarm_type;

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
			//将警报类型发送到警报类型的队列里去
			alarm_type = patient_data.Alarm_type;
			xQueueSend(Alarm_type_queue, &alarm_type, 0);

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
	PatientData_t Rx_patientdata_Receive;
    const TickType_t UPDATE_INTERVAL = pdMS_TO_TICKS(100);
	
	//在队列集中选中对应的队列的句柄
	QueueSetMemberHandle_t member_handle; 
    
	OLED_Printf(0, 0, OLED_6X8, "Name:");
	OLED_Printf(0, 10, OLED_6X8, "Age:");
	OLED_Printf(0, 20, OLED_6X8, "Sex:");
	OLED_Printf(0, 30, OLED_6X8, "Bed num:");
	OLED_Printf(0, 40, OLED_6X8, "Drip Rate:");

    while(1)
    {    
		//从队列集中获取有效信息
		member_handle = xQueueSelectFromSet(queueSet_handle, pdMS_TO_TICKS(100));

		if(member_handle == Patient_data_queue)
		{
			if(xQueueReceive(Patient_data_queue, &PatientData_Receive, 0) == pdTRUE)
			{
				g_Drops_per_minute = PatientData_Receive.Drops_per_minute;
				OLED_ShowNum(62, 30, PatientData_Receive.Bed_Num, 1, OLED_6X8);
				OLED_ShowNum(62, 40, PatientData_Receive.Drops_per_minute, 3, OLED_6X8);
				
				//显示报警状态
				if(PatientData_Receive.Drops_per_minute > ALARM_HIGH) 
				{
					OLED_Printf(0, 50, OLED_6X8, "                ");
					OLED_Printf(0, 50, OLED_6X8, "ALARM: HIGH");
				} 
				else if(PatientData_Receive.Drops_per_minute < ALARM_LOW) 
				{
					OLED_Printf(0, 50, OLED_6X8, "                ");
					OLED_Printf(0, 50, OLED_6X8, "ALARM: LOW");
				}
				else
				{
					OLED_Printf(0, 50, OLED_6X8, "                ");
					OLED_Printf(0, 50, OLED_6X8, "Normal speed");
				}
			}
        }
		//如果是返回的句柄是来自串口的数据, 就执行以下操作
		else if(member_handle == Rx_patientdata_queue)
		{
			if(xQueueReceive(Rx_patientdata_queue, &Rx_patientdata_Receive, 0) == pdTRUE)
			{
				OLED_Printf(62, 0,  OLED_6X8, "           ");
				OLED_Printf(62, 10, OLED_6X8, "           ");
				OLED_Printf(62, 20, OLED_6X8, "           ");
				OLED_ShowString(62, 0, Serial_RxPacket_Name, OLED_6X8);
				OLED_ShowNum(62, 10, *Serial_RxPacket_Age, 2, OLED_6X8);
				OLED_ShowString(62, 20, Serial_RxPacket_Sex, OLED_6X8);
			}
		}

		//定期更新显示（即使没有新数据）
        if((xTaskGetTickCount() - last_update_time) >= UPDATE_INTERVAL) 
        {
            OLED_Update();
            last_update_time = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
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
    uint8_t alarm_count = 0;
	uint8_t alarm_type;
    const uint8_t ALARM_THRESHOLD = 3;  // 连续3次报警才触发
	
    while(1)
    {
        if(xQueueReceive(Alarm_type_queue, &alarm_type, portMAX_DELAY) == pdTRUE)
        {
            // 报警逻辑处理
            switch(alarm_type) 
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
            // if(alarm_count >= ALARM_THRESHOLD && patient_data.Alarm_type != 0) 
			// {
            //     // 可以在这里添加日志记录功能
            // }
        }
    }
}

/*
 * @brief  串口任务, 在PC端上输入患者的信息
 * @param  无
 * @retval 无
 */
void SERIAL_TASK(void *pvParameters)
{
    PatientData_t Rx_patientdata;
	while(1)
	{
        if(Serial_RxFlag == 1)
        {
			
		}
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/*
 * @brief  串口接收消息中断, 用于接收PC端通过串口发送病人的信息
 * @param  无
 * @retval 无
 */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)	//判断是否是USART1的接收事件触发的中断
	{
		uint8_t RxData = USART_ReceiveData(USART1);			//读取数据寄存器，存放在接收的数据变量
		
		static uint8_t RxState = wait_header;
        static uint8_t pRxPacket = 0;
        static uint8_t HeaderIndex = 0;      //包头匹配的索引
        static uint8_t CurrentHeader = 0;    //当前匹配的包头类型 0:NAME, 1:AGE, 2:SEX
		static uint8_t age_temp = 0;

		switch(RxState)
		{
			case wait_header:
				//如果接收到第一个字符为'N', 就将当前包头的类型匹配为0, 并且状态转移至匹配name部分
				if(RxData == 'N')
				{
					CurrentHeader = 0;
					HeaderIndex = 1;
					RxState = match_name;
				}
				if(RxData == 'A')
				{
					CurrentHeader = 1;
					HeaderIndex = 1;
					RxState = match_age;
				}
				if(RxData == 'S')
				{
					CurrentHeader = 2;
					HeaderIndex = 1;
					RxState = match_sex;
				}
				break;
			case match_name:
				if(RxData == HEADER_NAME[HeaderIndex])
				{
					HeaderIndex++;
					//进行状态转移并且将接收数据数组的指针指到数组的开头, 准备接收数据
					if(HEADER_NAME[HeaderIndex] == '\0')
                    {
                        RxState = receive_data;
                        pRxPacket = 0;
                    }
				}
				else
                {
                    RxState = wait_header;  //匹配失败，重新开始
                }
				break;
			case match_age:
				if(RxData == HEADER_AGE[HeaderIndex])
				{
					HeaderIndex++;
					//进行状态转移并且将接收数据数组的指针指到数组的开头, 准备接收数据
					if(HEADER_AGE[HeaderIndex] == '\0')
                    {
                        RxState = receive_data;
                        pRxPacket = 0;
						age_temp = 0; 
                    }
				}
				else
                {
                    RxState = wait_header;  //匹配失败，重新开始
                }
				break;
			case match_sex:
				if(RxData == HEADER_SEX[HeaderIndex])
				{
					HeaderIndex++;
					//进行状态转移并且将接收数据数组的指针指到数组的开头, 准备接收数据
					if(HEADER_SEX[HeaderIndex] == '\0')
                    {
                        RxState = receive_data;
                        pRxPacket = 0;
                    }
				}
				else
                {
                    RxState = wait_header;  //匹配失败，重新开始
                }
				break;
			case receive_data:
				if(RxData == '\r')
				{
					RxState = end;
					switch(CurrentHeader)
                    {
						case 0:
							Serial_RxPacket_Name[pRxPacket] = '\0';
						break;
						case 1:
							Serial_RxPacket_Age[0] = age_temp;
							Serial_RxPacket_Age[pRxPacket] = '\0';
							age_temp = 0; 
						break;
						case 2:
							Serial_RxPacket_Sex[pRxPacket] = '\0';
						break;
					}
				}
				else
				{
					switch(CurrentHeader)
					{
						case 0:
							//这里用if的原因是, 每一次进来中断只能传入一个字符, 如果使用for循环的话, 偏移了之后就没有数据了
							if(pRxPacket < sizeof(Serial_RxPacket_Name) - 1)
							{
								Serial_RxPacket_Name[pRxPacket] = RxData;
								pRxPacket++;
							}
							break;
						case 1:
                            if(RxData >= '0' && RxData <= '9')
                            {
    						    age_temp = age_temp * 10 + (RxData - '0');
							}
							pRxPacket++;
							break;
						case 2:
							//这里用if的原因是, 每一次进来中断只能传入一个字符, 如果使用for循环的话, 偏移了之后就没有数据了
							if(pRxPacket < sizeof(Serial_RxPacket_Sex) - 1)
							{
								Serial_RxPacket_Sex[pRxPacket] = RxData;
								pRxPacket++;
							}
							break;
					}
				}
				break;
			case end:
				//如果收到'\n'字符, 就将接收到数据的标志位置1, 并回到等到包头的状态
				if(RxData == '\n')
				{
					Serial_RxFlag = 1;
					RxState = wait_header;
				}
				break;
		}
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		//清除标志位
	}
}

