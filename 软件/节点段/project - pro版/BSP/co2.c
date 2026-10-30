#include "co2.h"
#include "delay.h"
#include "stdio.h"
#include "string.h"
#include "UART.h"
#include "OLED.h"
extern UART_HandleTypeDef huart3;
#define CO2_UART		huart3
extern uint8_t uart3_Buff[9];
extern uint16_t uart3_i;
extern uint8_t rxdata3;
extern uint8_t rx3_flag, rx3_state;


void CO2_Init(void)
{
  HAL_UART_Receive_IT(&CO2_UART, &rxdata3, 1);
}
uint32_t value;
uint32_t Co2_GetValue(void)
{
	if (rx3_flag == 1)
	{
		//B1(0x2C)=uart3_Buff[0], B2(CO2高位)=uart3_Buff[1], B3(CO2低位)=uart3_Buff[2]
		value = (uart3_Buff[1] * 256) + (uart3_Buff[2]);
		rx3_flag = 0;
	}
	 return value;
}





