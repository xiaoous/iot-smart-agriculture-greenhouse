#include "ESP32S3.h"
#include "OLED.h"
#include "stdlib.h"
#include <string.h>
#include <stdio.h>
#include "UART.h"
#include "delay.h"
extern uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart1_i;
extern uint8_t rxdata1;
extern UART_HandleTypeDef huart1;
extern uint8_t uart1_rx_buff[RXBUFF_MAX_SIZEOF], uart1_rx_state, uart1_rx_flag;
extern uint8_t uart1_rx_i;

#define esp32_cnt  uart1_i
#define esp32_buf  uart1_Buff
unsigned short esp32_cntPre = 0;

void ESP32S3_Clear(void)
{
	memset(esp32_buf, 0, sizeof(esp32_buf));
	esp32_cnt = 0;
}

void Uart1_SendString(char *s)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), 0xffff);
}

void ESP32S3_Init(void)
{
	HAL_UART_Receive_IT(&huart1, &rxdata1, 1);
	// ESP32S3不需要AT命令初始化，直接开始通信
	Delay_ms(1000);
}

/* ???????? */
void ESP32S3_SendData(char *str)
{
	Uart1_SendString(str);
	Delay_ms(50);
}

#define RXERROR -1  // 定义错误码

int16_t Received_Data_Processing(const char *str, uint8_t len) {
    char temp[10] = {0};  // 初始化数组，确保结束符
    uint8_t temp_i = 0;
    char *p1 = NULL;

    // 在缓冲区中查找目标字段
    if ((p1 = strstr((char *)uart1_rx_buff, str)) != NULL) {
        // 找到冒号的位置
        p1 += strlen(str);
        if (*p1 == ':') {
            p1++;  // 跳过冒号

            // 跳过冒号后的空格（如果有）
            while (*p1 == ' ' || *p1 == '\t') {
                p1++;
            }

            // 读取值，直到遇到逗号或右大括号
            while (*p1 != ',' && *p1 != '}' && temp_i < 9) {  // 防止数组溢出
                temp[temp_i++] = *p1;
                p1++;
            }
            temp[temp_i] = '\0';  // 确保字符串结束符

            return atoi(temp);  // 将字符串转换为整数
        }
    }
    return ESP32S3_RXERROR;  // 未找到字段
}

extern uint8_t mode;
extern uint8_t fan_state_1, water_state_1, canopy_state;
extern uint8_t fan_state_2, water_state_2, pump_state;
extern uint8_t temp_max_1, humi_min_1, light_min;
extern uint8_t temp_max_2, humi_min_2, t_humi_min;
extern float t_ph_max;
extern uint32_t CO2_max;

int8_t ESP32S3_GetIPD(unsigned short timeOut)
{
  int16_t value;

		if (uart1_rx_flag == 1)
		{
			OLED_Clear();
			value = Received_Data_Processing("mode", strlen("mode"));
			if (value != ESP32S3_RXERROR)
				mode = value;
			if(mode == 2)
			{
				value = Received_Data_Processing("temp_max_1", strlen("temp_max_1"));
				if (value != ESP32S3_RXERROR)
					temp_max_1 = value;
				value = Received_Data_Processing("kq_humi_min_1", strlen("kq_humi_min_1"));
				if (value != ESP32S3_RXERROR)
					humi_min_1 = value;
				value = Received_Data_Processing("light_min", strlen("light_min"));
				if (value != ESP32S3_RXERROR)
					light_min = value;
				value = Received_Data_Processing("CO2_max", strlen("CO2_max"));
				if (value != ESP32S3_RXERROR)
					CO2_max = value;
			}
			else if(mode == 3)
			{
				value = Received_Data_Processing("fan_state_1", strlen("fan_state_1"));
				if (value != ESP32S3_RXERROR)
					fan_state_1 = value;
				value = Received_Data_Processing("water_state_1", strlen("water_state_1"));
				if (value != ESP32S3_RXERROR)
					water_state_1 = value;
				value = Received_Data_Processing("canopy_state", strlen("canopy_state"));
				if (value != ESP32S3_RXERROR)
					canopy_state = value;
			}
			else if(mode == 5)
			{
				value = Received_Data_Processing("temp_max_2", strlen("temp_max_2"));
				if (value != ESP32S3_RXERROR)
						temp_max_2 = value;
				value = Received_Data_Processing("kq_humi_min_2", strlen("kq_humi_min_2"));
				if (value != ESP32S3_RXERROR)
						humi_min_2 = value;
				value = Received_Data_Processing("t_humi_min", strlen("t_humi_min"));
				if (value != ESP32S3_RXERROR)
						t_humi_min = value;
				value = Received_Data_Processing("t_ph_max", strlen("t_ph_max"));
				if (value != ESP32S3_RXERROR)
						t_ph_max = value;
				value = Received_Data_Processing("light_min", strlen("light_min"));
				if (value != ESP32S3_RXERROR)
						light_min = value;
				value = Received_Data_Processing("CO2_max", strlen("CO2_max"));  // 改为小写
				if (value != ESP32S3_RXERROR)
						CO2_max = value;
			}
			else if(mode == 6)
			{
				value = Received_Data_Processing("fan_state_2", strlen("fan_state_2"));
				if (value != ESP32S3_RXERROR)
					fan_state_2 = value;
				value = Received_Data_Processing("water_state_2", strlen("water_state_2"));
				if (value != ESP32S3_RXERROR)
					water_state_2 = value;
				value = Received_Data_Processing("pump_state", strlen("pump_state"));
				if (value != ESP32S3_RXERROR)
					pump_state = value;
			}
			
			uart1_rx_flag = 0;
			return ESP32S3_REV_OK;
		}
	
	return ESP32S3_RXERROR;														//?????δ?????????????

}

