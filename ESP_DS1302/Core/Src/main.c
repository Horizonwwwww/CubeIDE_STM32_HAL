/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "oled.h"
#include "font.h"
#include "esp.h"
#include "dht11.h"
#include "ds1302.h"

#include "cJSON.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PUMP_IA_PIN GPIO_PIN_0
#define PUMP_IA_PORT GPIOB
#define PUMP_IB_PIN GPIO_PIN_1
#define PUMP_IB_PORT GPIOB

#define BUZZER_PIN GPIO_PIN_15
#define BUZZER_PORT GPIOB
#define BEEP_DURATION 20  // 蜂鸣持续时间(ms)

#define WIFI_SSID "31442"
#define WIFI_PASS "dianqi442"

//#define WIFI_SSID "Niko"
//#define WIFI_PASS "12345678lk"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t uart2_rx_data[256] = {0};  // 专门用于UART2接收

char R_data[1024];			//定义结束数据的缓冲区
uint8_t R_length=0;			//定义结束数据的长度
uint8_t R_i=0;					//由于遍历缓冲区数据
char request_id[128];		//命令下发的request_id
uint8_t aRxBuffer1;			//一个字节的缓冲


// 添加状态变量
typedef enum {
    CMD_IDLE,
    CMD_SG90_1,      // 快速转动
    CMD_SG90_2,      // 保持2秒
    CMD_PUMP_1,      // 水泵2秒
    CMD_PUMP_2       // 水泵8秒
} CommandState;

volatile CommandState currentCmd = CMD_IDLE;
uint32_t cmdStartTime = 0;
uint32_t cmdDuration = 0;


DS1302_Time currentTime;
uint32_t lastTimeUpdate = 0;
typedef enum {
    PUMP_IDLE,
    PUMP_RUNNING,
    PUMP_WAITING_24H
} PumpState;

PumpState pumpState = PUMP_IDLE;
uint32_t pumpTimer = 0;
uint8_t lastPumpDay = 0;// 记录上次启动水泵的日期


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Pump_Start(void);
void Pump_Stop(void);
void Pump_Reverse(void);
void Servo_SetAngle(uint8_t angle);

void ParseMQTTData(char* data, int length);

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);


void Beep(uint32_t duration);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Beep(uint32_t duration) {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET); // 低电平触发
    HAL_Delay(duration);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);   // 恢复高电平
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */





  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;


  OLED_Init();

  OLED_NewFrame();
  OLED_PrintString(0,5,"LK", &font16x16, OLED_COLOR_NORMAL);
  OLED_ShowFrame();



  // 使用Ex函数，接收不定长数据
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_rx_data, sizeof(uart2_rx_data));
  // 关闭DMA传输过半中断（HAL库默认开启，但只需要接收完成中断）
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  // 现在等待数据接收完成，接收完成后会自动进入回调函数...




  HAL_UART_Receive_IT(&huart1,&aRxBuffer1,1); //使能串口接收中断


  ESP8266_Init(&huart1);
//  ESP8266_ConnectToWiFi(WIFI_SSID, WIFI_PASS);
  ESP8266_ConnectToHuaweiCloud();

  // 启动TIM4 PWM通道4（PB9）
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  Servo_SetAngle(0);  // 初始位置设为90°


  DS1302_Init();
//  DS1302_ForceInitTime(); // 只执行一次


  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);


  uint32_t lastSensorUpdate = 0;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	    // 按固定周期上报传感器数据
	    if (HAL_GetTick() - lastSensorUpdate >= 2000)
	    {
	        DHT11_Data dht_data;
	        if (DHT11_Read(&dht_data) && dht_data.valid) {
	            // OLED显示代码
	        	OLED_ClearArea(0, 0, 128, 32);
		        char temp_str[20];
		        sprintf(temp_str, "Temp: %d.%dC", dht_data.temp_int, dht_data.temp_decimal);
		        OLED_PrintString(0, 0, temp_str, &font16x16, OLED_COLOR_NORMAL);

		        char hum_str[20];
		        sprintf(hum_str, "Hum: %d.%d%%", dht_data.humidity_int, dht_data.humidity_decimal);
		        OLED_PrintString(0, 16, hum_str, &font16x16, OLED_COLOR_NORMAL);
		        OLED_ShowFrame();
	            // 发送传感器数据
	            ESP8266_SendSensorData(dht_data.temp_int, dht_data.temp_decimal,
	                                 dht_data.humidity_int, dht_data.humidity_decimal);
//	    	    HAL_Delay(500);  // 适当延时减少CPU占用
	            lastSensorUpdate = HAL_GetTick();
	        }
	    }

	    // 处理当前命令状态
	    switch(currentCmd)
	    {
	        case CMD_SG90_1:
	            if(HAL_GetTick() - cmdStartTime >= 1000)
	            {
	                Servo_SetAngle(0);
	                currentCmd = CMD_IDLE;

	            }
	            Beep(BEEP_DURATION);
	            break;

	        case CMD_SG90_2:
	            if(HAL_GetTick() - cmdStartTime >= 2000)
	            {
	                Servo_SetAngle(0);
	                currentCmd = CMD_IDLE;

	            }
	            Beep(BEEP_DURATION);
	            break;

	        case CMD_PUMP_1:
	            if(HAL_GetTick() - cmdStartTime >= 2000)
	            {
	                Pump_Stop();
	                currentCmd = CMD_IDLE;

	            }
	            Beep(BEEP_DURATION);
	            break;

	        case CMD_PUMP_2:
	            if(HAL_GetTick() - cmdStartTime >= 8000)
	            {
	                Pump_Stop();
	                currentCmd = CMD_IDLE;

	            }
	            Beep(BEEP_DURATION);
	            break;

	        case CMD_IDLE:
	        default:
	            // 无命令执行时不做任何操作
	            break;
	    }

	    // 每秒更新时间显示
	    if (HAL_GetTick() - lastTimeUpdate >= 1000) {
	        DS1302_GetTime(&currentTime);

	        // OLED显示时间
        	OLED_ClearArea(0, 32, 128, 32);
	        char time_str[20];
	        sprintf(time_str, "%02d:%02d:%02d",
	                currentTime.hours,
	                currentTime.minutes,
	                currentTime.seconds);
	        OLED_PrintString(0, 32, time_str, &font16x16, OLED_COLOR_NORMAL);

	        char date_str[20];
	        sprintf(date_str, "20%02d-%02d-%02d",
	                currentTime.year,
	                currentTime.month,
	                currentTime.day);
	        OLED_PrintString(0, 48, date_str, &font16x16, OLED_COLOR_NORMAL);
	        OLED_ShowFrame();

	        lastTimeUpdate = HAL_GetTick();
	    }


	    // 每天定时启动水泵3秒
	    switch(pumpState) {
	        case PUMP_IDLE:
	            // 检查当前时间，并且不是同一天已经执行过
	            if (currentTime.hours == 18 &&
	                currentTime.minutes == 50 &&
	                currentTime.seconds < 3 &&
	                lastPumpDay != currentTime.day)
	            {
	                Pump_Start();
	                pumpState = PUMP_RUNNING;
	                pumpTimer = HAL_GetTick();
	                lastPumpDay = currentTime.day; // 记录执行日期
	                Beep(BEEP_DURATION);
	            }
	            break;

	        case PUMP_RUNNING:
	            if (HAL_GetTick() - pumpTimer >= 3000) { // 运行3秒
	                Pump_Stop();
	                pumpState = PUMP_WAITING_24H;
	            }
	            break;

	        case PUMP_WAITING_24H:
	            // 等待时间过去，防止重复执行
	            if (!(currentTime.hours == 18 && currentTime.minutes == 50)) {
	                pumpState = PUMP_IDLE;
	            }
	            break;
	    }



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void Pump_Start(void) {
    HAL_GPIO_WritePin(PUMP_IA_PORT, PUMP_IA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PUMP_IB_PORT, PUMP_IB_PIN, GPIO_PIN_RESET);
}

void Pump_Stop(void) {
    HAL_GPIO_WritePin(PUMP_IA_PORT, PUMP_IA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PUMP_IB_PORT, PUMP_IB_PIN, GPIO_PIN_RESET);
}

void Pump_Reverse(void) {
    HAL_GPIO_WritePin(PUMP_IA_PORT, PUMP_IA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PUMP_IB_PORT, PUMP_IB_PIN, GPIO_PIN_SET);
}


/* 舵机控制函数（角度范围0-180°） */
void Servo_SetAngle(uint8_t angle) {
    // 限制角度范围
    angle = (angle > 180) ? 180 : angle;

    // 转换为PWM脉宽值（500-2500对应0-180°）
    uint16_t pulse = 500 + angle * (2000 / 180);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
}





void ParseMQTTData(char* data, int length)
{

		HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nRX Data Len: ", 15, 100);
	    char len_str[10];
	    sprintf(len_str, "%d", length);
	    HAL_UART_Transmit(&huart2, (uint8_t*)len_str, strlen(len_str), 100);
	    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nRX Data: ", 11, 100);
	    HAL_UART_Transmit(&huart2, (uint8_t*)data, length, 100);

		if(length>120)//接收数据
		{



	    // 1. 提取 request_id
	    char* rid_start = strstr(data, "request_id=");
	    if (!rid_start) {
	        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nNo request_id found\r\n", 24, 100);
	        return;
	    }

	    char request_id[64] = {0};
	    sscanf(rid_start, "request_id=%36[^\"]", request_id);

	    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nMQTT request_id: ", 19, 100);
	    HAL_UART_Transmit(&huart2, (uint8_t*)request_id, strlen(request_id), 100);

	    // 2. 提取 JSON 字符串
	    char* json_start = strchr(data, '{');
	    if (!json_start) {
	        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nNo JSON found\r\n", 17, 100);
	        return;
	    }

	    cJSON* root = cJSON_Parse(json_start);
	    if (!root) {
	        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nJSON Parse Error\r\n", 19, 100);
	        return;
	    }

	    cJSON* paras = cJSON_GetObjectItem(root, "paras");
	    if (paras) {
	        cJSON* led_cmd = cJSON_GetObjectItem(paras, "LED_CMD");
	        if (cJSON_IsNumber(led_cmd))
	        {
	            if (led_cmd->valueint == 0)
	            {
	                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	            } else if (led_cmd->valueint == 1)
	            {
	                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	            }
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nLED CMD executed\r\n", 20, 100);
	            ESP8266_response(request_id);
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nResponse Done\r\n", 17, 100);
	        }

	        cJSON* sg90_cmd = cJSON_GetObjectItem(paras, "SG90_CMD");
	        if (cJSON_IsNumber(sg90_cmd))
	        {
	            if (sg90_cmd->valueint == 1)
	            {
	                // 正转90°然后立即复位
	                currentCmd = CMD_SG90_1;
	                cmdStartTime = HAL_GetTick();
	                Servo_SetAngle(90);

	            } else if (sg90_cmd->valueint == 2)
	            {
	                // 正转90°保持2秒后复位
	                currentCmd = CMD_SG90_2;
	                cmdStartTime = HAL_GetTick();
	                Servo_SetAngle(90);

	            }
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nSG90 CMD executed\r\n", 20, 100);
	            ESP8266_response(request_id);
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nResponse Done\r\n", 17, 100);
	        }

	        cJSON* jt_cmd = cJSON_GetObjectItem(paras, "JT_CMD");
	        if (cJSON_IsNumber(jt_cmd))
	        {
	            if (jt_cmd->valueint == 1)
	            {
	                // 水泵启动2秒后停止
	                currentCmd = CMD_PUMP_1;
	                cmdStartTime = HAL_GetTick();
	                Pump_Start();

	            } else if (jt_cmd->valueint == 2)
	            {
	                // 水泵启动8秒后停止
	                currentCmd = CMD_PUMP_2;
	                cmdStartTime = HAL_GetTick();
	                Pump_Start();

	            }
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nJT CMD executed\r\n", 20, 100);
	            ESP8266_response(request_id);
	            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nResponse Done\r\n", 17, 100);
	        }
	    }

	    cJSON_Delete(root); // 清理 JSON 内存
		}
}


// 不定长数据接收完成回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    // 使用DMA将接收到的数据发送回去
    HAL_UART_Transmit_DMA(&huart2, uart2_rx_data, Size);
    // 重新启动接收，使用Ex函数，接收不定长数据
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_rx_data, sizeof(uart2_rx_data));
    // 关闭DMA传输过半中断
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        HAL_UART_Receive_IT(&huart1, &aRxBuffer1, 1);
        // save char from uart receive
        if(aRxBuffer1 != '\n')
            R_data[R_length++] = aRxBuffer1;
        else
        {
            ParseMQTTData(R_data, R_length);
            R_length = 0;
            memset(R_data, 0, sizeof(R_data));
        }
    }


}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
