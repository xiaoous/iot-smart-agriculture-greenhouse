/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
	                   日期：2025-10-25 创改
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "delay.h"
#include "OLED.h"
#include "CO2.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "zigbee.h"
#include "DHT11.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
char str[200];
char s[200];
uint8_t mode;
uint8_t temp_value_2, humi_value_2, t_humi_value,light;
float t_ph_value;
uint8_t fan_state_2, water_state_2, pump_state, buzzer_state_2;
uint32_t CO2_value;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
*风扇控制函数
*我们的外设没有这个模块
*
*/
//void Fan_Set_State(uint8_t state)
//{
//    fan_state_2 = state;
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (GPIO_PinState)state);
//}

/*
*水雾控制函数
*/
void Water_Set_State(uint8_t state)
{
    water_state_2 = state;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, (GPIO_PinState)state);
}

/*
*	水泵控制函数
*/
void Pump_Set_State(uint8_t state)
{
    pump_state = state;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (GPIO_PinState)state);
}

/*
*蜂鸣器控制函数
*/
void Buzzer_Set_State(uint8_t state)
{
    buzzer_state_2 = state;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (GPIO_PinState)state);
}

/*
*	光照传感器(PA0)
* 土壤湿度传感器(PA1)
* PH传感器(PA4)
***一个传感器一个通道互不干扰。
*/
uint8_t adc_time;
uint8_t ADC_GetValue(void)
{
    uint8_t i;
    uint32_t adc_sum_buff[3] = {0, 0, 0};  // 三个通道的缓冲区
    uint16_t adc_buff[3] = {0, 0, 0};
    if (adc_time < 5)
    {
        adc_time++;
        return 0;
    }
    adc_time = 0;
    
    // 通过DMA采集三个通道的值
    for (i = 0; i < 10; i++)
    {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buff, 3);  // 三个通道
        adc_sum_buff[0] += adc_buff[0];  // 光照传感器(PA0)
        adc_sum_buff[1] += adc_buff[1];  // 土壤湿度传感器(PA1)
        adc_sum_buff[2] += adc_buff[2];  // PH传感器(PA4)
    }
    
    // 计算平均值
    uint16_t avg_light_adc = adc_sum_buff[0] / 10;
    uint16_t avg_humi_adc = adc_sum_buff[1] / 10;  // 土壤湿度ADC值
    uint16_t avg_ph_adc = adc_sum_buff[2] / 10;    // PH值ADC值
    
    // 调试：保存原始ADC值到全局变量以便在主循环中显示
    //sprintf(s, "ADC:%d ", avg_light_adc);  // 存储ADC原始值到s字符串
    
    // 光敏传感器可能是反向关系（光线越强，ADC值越小）
    light = 46+(uint8_t)((1.0 - (avg_light_adc / 4095.0)) * 100);  // 反向映射到0-100
    
    // 确保值在有效范围内
    if (light > 100) light = 98;
    if (light < 48) light = 0;
    
    // 使用独立的通道计算土壤湿度和PH值
    // 土壤湿度使用PA1通道的值
    t_humi_value = 90 - (avg_humi_adc * 100 / 4095.0);  // 土壤湿度
    if(t_humi_value>10)
			t_humi_value+=15;
		
		// PH值使用PA4通道的值
    t_ph_value = ((avg_ph_adc / 4095.0) * 9) * 0.6 + 2.2;  // PH值水PH==7左右
    return 1;
}

/*
* 统一控制函数
*/
void Ctrl(void)
{
//    Fan_Set_State(fan_state_2);
    Water_Set_State(water_state_2);
    Pump_Set_State(pump_state);
    Buzzer_Set_State(buzzer_state_2);
}
/*
*	OLED显示
*	节点二没有OLED。
*	
*/
void Page(void)
{
//        OLED_ShowChinese(1, 1, "�ڵ��");
//        OLED_ShowChinese(1, 16, "�¶ȣ�");
//        OLED_ShowNum(40, 16, temp_value_2, 2, 8);
//        OLED_ShowChinese(65, 16, "ʪ�ȣ�");
//        OLED_ShowNum(105, 16, humi_value_2, 2, 8);
//        OLED_ShowChinese(1, 32, "���ȣ�");
//        OLED_ShowFloatNum(60, 32, t_ph_value, 2, 1, 8);
//        OLED_ShowChinese(1, 48, "����ʪ�ȣ�");
//        OLED_ShowNum(80, 48, t_humi_value, 3, 8);
//				OLED_ShowChinese(1, 64, "����ǿ��:");
//				OLED_ShowNum(48, 64, light, 4, 8);
//				sprintf(s, "CO2:%d        ", CO2_value);
//				OLED_ShowString(1, 80, s, 8);
//				
}

/*
*	向ZIgbee发送数据
*	
*	
*/
uint8_t time_out;
void Send_Data(void)
{
    if(time_out < 20)
    {
        time_out ++;
        return ;
    }
    time_out = 0;
	sprintf(str,"{temp_value_2:%d,kq_humi_value_2:%d,t_ph_value:%.1f,t_humi_value:%d,light:%d,CO2_value:%d}\n",temp_value_2, humi_value_2, t_ph_value, t_humi_value, light , CO2_value);
    Zigbee_SendString(str);
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
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
	  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  OLED_Init();
  Zigbee_Init();
  DHT11_Init();
	CO2_Init();
//  Fan_Set_State(0);
  Water_Set_State(0);
  Pump_Set_State(0);
  Buzzer_Set_State(0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		Zigbee_Recv_Data();
        Send_Data();
	CO2_value = Co2_GetValue();
        ADC_GetValue();
        DHT11_Read_Data(&humi_value_2, &temp_value_2);
		Ctrl();
        Page();
		OLED_Update();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
