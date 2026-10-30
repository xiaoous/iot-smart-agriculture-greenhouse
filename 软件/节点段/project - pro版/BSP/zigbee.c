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

int16_t ZigbeeReceived_Data_Processing(const char *str, uint8_t len)						/* 处理接收的数据 */
{
	char temp[10];
	uint8_t temp_i = 0;
	char *p1 = NULL;
	if ((p1 = strstr((char *)rx_buff, str)) != NULL)													/* 通过关键字筛选有用的数据 */
	{
		p1 += strlen(str) + 1;
		while(*p1 != ',')
		{
			temp[temp_i++] = *p1;
			p1++;
		}
		return atoi(temp);
	}
	return -1;
}

extern uint8_t fan_state_2, water_state_2, pump_state, buzzer_state_2;

void Zigbee_Recv_Data(void)
{
	int value;
	if (rx_flag == 1)
	{
		value = ZigbeeReceived_Data_Processing("fan_state_2", strlen("fan_state_2"));
		if (value != -1)
			fan_state_2 = value;
        
        value = ZigbeeReceived_Data_Processing("water_state_2", strlen("water_state_2"));
		if (value != -1)
			water_state_2 = value;
        
        value = ZigbeeReceived_Data_Processing("pump_state", strlen("pump_state"));
		if (value != -1)
			pump_state = value;
        
        value = ZigbeeReceived_Data_Processing("buzzer_state_2", strlen("buzzer_state_2"));
		if (value != -1)
			buzzer_state_2 = value;
		rx_flag = 0;
	}
}

