#include "zigbee.h"
#include "OLED.h"
#include "stdlib.h"
#include <string.h>
#include <stdio.h>
#include "UART.h"
extern uint8_t rx_buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart2_i;
extern uint8_t rxdata2;
extern UART_HandleTypeDef huart2;
extern uint8_t buzzer_state, rx_flag;


void Zigbee_SendString(char *s)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), 0xffff);
}
void Zigbee_Init(void)
{
	HAL_UART_Receive_IT(&huart2, &rxdata2, 1);

}

float ZigbeeReceived_Data_Processing(const char *keyword)						/* 处理接收到的数据 */
{
	char temp[10];
	uint8_t temp_i = 0;
	char *p1 = NULL;
	// 直接使用USART2的接收缓冲区进行处理
	if ((p1 = strstr((char *)uart2_rx_buff, keyword)) != NULL)										/* 通过关键词筛选有用的数据 */
	{
		// 确保找到的是完整的关键词，而不是其他关键词的一部分
		// 检查关键词后面是否是冒号或等号
		p1 += strlen(keyword);
		while(*p1 != ':' && *p1 != '=' && *p1 != '\0') {
			p1++;
		}
		if(*p1 == '\0') { // 没有找到分隔符，返回错误
			return -1;
		}
		p1++; // 跳过分隔符(:或=)
		
		// 提取数据直到遇到逗号或右大括号
		while(*p1 != ',' && *p1 != '}' && *p1 != '\0' && temp_i < sizeof(temp) - 1)
		{
			// 跳过可能的空格
			if(*p1 == ' ' && temp_i == 0) {
				p1++;
				continue;
			}
			temp[temp_i++] = *p1;
			p1++;
		}
		temp[temp_i] = '\0'; // 确保字符串正确终止
		
		// 检查是否提取到了有效数据
		if(temp_i == 0) {
			return -1;
		}
		
		return atof(temp);
	}
	return -1;
}
extern uint8_t temp_value_1, humi_value_1, light, canopy_flag;
extern uint8_t temp_value_2, humi_value_2, t_humi_value;
extern float t_ph_value;
extern uint32_t CO2_value;

void Zigbee_Recv_Data(void)
{
	int value;
	// 使用USART2特定的标志
	if (uart2_rx_flag == 1)
	{
		value = ZigbeeReceived_Data_Processing("temp_value_2");
		if (value != -1)
			temp_value_2 = value;
        
        value = ZigbeeReceived_Data_Processing("kq_humi_value_2");
		if (value != -1)
			humi_value_2 = value;
        
        value = ZigbeeReceived_Data_Processing("t_humi_value");
		if (value != -1)
			t_humi_value = value;
        
        float F_value = ZigbeeReceived_Data_Processing("t_ph_value");
        if (F_value != -1) {
            t_ph_value = F_value;
        }
        
         value = ZigbeeReceived_Data_Processing("temp_value_1");
		if (value != -1)
			temp_value_1 = value;
        
         value = ZigbeeReceived_Data_Processing("kq_humi_value_1");
		if (value != -1)
			humi_value_1 = value;
        
         value = ZigbeeReceived_Data_Processing("CO2_value");
		if (value != -1)
			CO2_value = value;
        
         value = ZigbeeReceived_Data_Processing("light");
		if (value != -1)
			light = value;
        
        value = ZigbeeReceived_Data_Processing("canopy_flag");
		if (value != -1)
			canopy_flag = value;
		// 清除USART2特定的标志
		uart2_rx_flag = 0;
		// 同时清除全局标志以保持兼容性
		rx_flag = 0;
	}
}

