/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "gpio.h"
#include "usart.h"
#include "tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "KEY.h"
#include "Zigbee.h"
#include "OLED.h"
#include "ESP32S3.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
char str[500];
char s[200];
extern uint8_t thsl_i;
uint8_t mode=0;
uint8_t temp_value_1, humi_value_1, light;
uint8_t temp_value_2, humi_value_2, t_humi_value;
uint8_t fan_state_1, water_state_1, canopy_state, canopy_flag, buzzer_state_1, led_state_1;
uint8_t fan_state_2, water_state_2, pump_state, buzzer_state_2;
float t_ph_value;
uint32_t CO2_value;
uint8_t temp_max_1 = 40, humi_min_1 = 0, light_min = 50;
uint8_t temp_max_2 = 40, humi_min_2 = 20, t_humi_min = 0;
float t_ph_max = 10;
uint32_t CO2_max = 1000;
uint8_t warning;

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
void ESP32S3_Send_Data(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
*   Function: Control logic
*   Input: None
*   Return: None
*/
void Ctrl(void)
{
        if((mode == 0) || (mode == 1) || (mode == 4))
    {
        if((light < light_min) || (CO2_value > CO2_max) || (temp_value_2 > temp_max_2) || (humi_value_2 < humi_min_2) || (t_humi_value < t_humi_min) || (t_ph_value > t_ph_max))        {
            warning = (light < light_min) ? 3 : (CO2_value > CO2_max) ? 4 : (temp_value_2 > temp_max_2) ? 5 : (humi_value_2 < humi_min_2) ? 6 : (t_humi_value < t_humi_min) ? 7 : 8;

            // Turn off
            buzzer_state_1 = 0;
            fan_state_1 = 0;
            canopy_state = 0;
            water_state_1 = 0;
            led_state_1 = 0;

            if((temp_value_2 > temp_max_2) || (humi_value_2 < humi_min_2) || (t_humi_value < t_humi_min) || (t_ph_value > t_ph_max) || (light < light_min) || (CO2_value > CO2_max))            {
                buzzer_state_2 = 1;
                if(temp_value_2 > temp_max_2)
                    fan_state_2 = 1;
                else
                    fan_state_2 = 0;

                if(humi_value_2 < humi_min_2)
                    water_state_2 = 1;
                else
                    water_state_2 = 0;

                if(t_humi_value < t_humi_min)
                    pump_state = 1;
                else
                    pump_state = 0;
                                // Send control command
            }
            else
            {
                buzzer_state_2 = 0;
                fan_state_2 = 0;
                water_state_2 = 0;
                pump_state = 0;
            }

        }
        else
        {
            warning = 0;
            buzzer_state_1 = 0;
            buzzer_state_2 = 0;
            led_state_1 = 0;
            fan_state_1 = 0;
            canopy_state = 0;
            water_state_1 = 0;
            fan_state_2 = 0;
            water_state_2 = 0;
            pump_state = 0;
        }
    }
}


/*
*   Function: Page display
*   Input: None
*   Return: None
*/
void Page(void)
{
        // ????
    if(mode == 0)
    {
        OLED_ShowChinese(1, 1, "协调器");
        OLED_ShowChinese(1, 16, "节点二");
        OLED_ShowChinese(100,16, "←");
    }
    // ????????
    else if(mode == 1)
    {
    OLED_ShowChinese(0, 0, "光照");  	
    OLED_ShowChinese(1, 16, "温度：");
    OLED_ShowNum(40, 16, temp_value_2, 2, 8);
    OLED_ShowChinese(65, 16, "湿度：");
    OLED_ShowNum(105, 16, humi_value_2, 2, 8);
		OLED_ShowString(1,32,"CO2:",OLED_8X16);
   // OLED_ShowChinese(1, 32, "CO2:");
    OLED_ShowNum(40, 32, CO2_value, 4, 8);
			
		OLED_ShowString(1,48,"PH",OLED_8X16);
    OLED_ShowChinese(17, 48, "值:");
			
    OLED_ShowFloatNum(35, 48, t_ph_value, 2, 1, 8);
    OLED_ShowChinese(78, 48, "土湿:"); 
    OLED_ShowNum(105, 48, t_humi_value, 2, 8);
    OLED_ShowNum(40, 0, light, 2, 8);
    }
    // ???3???????
    else if(mode == 2 || mode == 5)
    {
    // OLED_ShowChinese(1, 1, "大棚");      
    OLED_ShowChinese(0, 0, "温度：");
    OLED_ShowNum(40, 0, temp_max_2, 2, 8);  
    OLED_ShowChinese(80, 1, "阈值");
    OLED_ShowChinese(1, 16,"湿度：" );
    OLED_ShowNum(40, 16,  humi_min_2,2, 8);
    OLED_ShowChinese(65, 16, "光照：");
    OLED_ShowNum(105, 16,light_min,2, 8);
		OLED_ShowString(1,32,"CO2:",OLED_8X16);
    //OLED_ShowChinese(1, 32, "CO2:");
    OLED_ShowNum(40, 32, CO2_max, 4, 8);
  // OLED_ShowChinese(1, 48, "PH值：");
		OLED_ShowString(1,48,"PH",OLED_8X16);
    OLED_ShowChinese(17, 48, "值:");
    OLED_ShowFloatNum(35, 48, t_ph_max, 2, 1, 8);  
 
    OLED_ShowChinese(78, 48, "土湿：");
    OLED_ShowNum(105, 48, t_humi_min, 2, 8);	
    }
    // ??????迪?????
    else if(mode == 6)
    {
    OLED_ShowChinese(1, 1, "大棚");
    OLED_ShowChinese(80, 1, "手动");

    OLED_ShowChinese(1, 16, "水泵：");
    OLED_ShowChinese(40, 16, (water_state_2 == 1) ? "开" : "关");
  
    OLED_ShowChinese(1, 32, "水雾：");
    OLED_ShowChinese(40, 32, (pump_state == 1) ? "开" : "关");
    OLED_ShowChinese(80, (thsl_i + 1) * 16, "←"); 
    }
}
/*
*   Function: Send data
*   Input: None
*   Return: None
*/
uint8_t time_out;
void Zigbee_Send_Data(void)
{
    if(time_out < 10)
    {
        time_out++;
        return;
    }
    time_out = 0;
        sprintf(str, "{fan_state_2:%d, water_state_2:%d, pump_state:%d, buzzer_state_2:%d,}\r\n",
    fan_state_2, water_state_2, pump_state, buzzer_state_2);
        Zigbee_SendString(str);
}

/*
*   Function: Send data
*   Input: None
*   Return: None
*/
void ESP32S3_Send_Data(void)
{
    if(time_out < 10)
    {
        time_out++;
        return;
    }
    time_out = 0;
    sprintf(str, "{\"CO2_value\":%d,\"light\":%d,\"temp_value_2\":%d,\"kq_humi_value_2\":%d,\"t_humi_value\":%d,\"t_ph_value\":%.1f}\r\n",
     CO2_value, light, temp_value_2, humi_value_2, t_humi_value, t_ph_value);
        ESP32S3_SendData(str);
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
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  OLED_Init();

  KEY_Init();
  Zigbee_Init();
        ESP32S3_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

                Zigbee_Send_Data();
                Zigbee_Recv_Data();
        ESP32S3_GetIPD(100);
        ESP32S3_Send_Data();
                KEY_Scan();
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
