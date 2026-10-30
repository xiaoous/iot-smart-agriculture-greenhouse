#include "UART.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// USART1相关变量
uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart1_i;
uint8_t rxdata1;
uint8_t uart1_rx_buff[RXBUFF_MAX_SIZEOF];
uint8_t uart1_rx_state, uart1_rx_flag;
uint8_t uart1_rx_i;

// USART2相关变量（用于Zigbee）
uint8_t uart2_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart2_i;
uint8_t rxdata2;
uint8_t uart2_rx_buff[RXBUFF_MAX_SIZEOF];
uint8_t uart2_rx_state, uart2_rx_flag;
uint8_t uart2_rx_i;

// 全局变量（保持向后兼容）
uint8_t rx_buff[RXBUFF_MAX_SIZEOF], rx_state, rx_flag;
uint8_t rx_i;


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart1_Buff[uart1_i++] = rxdata1;
    HAL_UART_Receive_IT(&huart1, &rxdata1, 1);
    switch(uart1_rx_state)
    {
        case 0:
            // 当检测到左大括号时，重置接收索引并开始接收数据
            if (rxdata1 == '{')
            {
                uart1_rx_state = 1;
                uart1_rx_i = 0; // 重置索引
                memset(uart1_rx_buff, 0, RXBUFF_MAX_SIZEOF); // 清空接收缓冲区
            }
            break;
        case 1:
            if(rxdata1 == '}')
            {
                // 确保最后一个字符正确终止字符串
                uart1_rx_buff[uart1_rx_i] = '\0';
                uart1_rx_state = 0;
                uart1_rx_flag = 1;
                // 保持向后兼容
                rx_flag = 1;
            }
            else if(uart1_rx_i < RXBUFF_MAX_SIZEOF - 1)
                uart1_rx_buff[uart1_rx_i++] = rxdata1;
            break;
    }
    if (uart1_i >= RXBUFF_MAX_SIZEOF)
    {
      uart1_i = 0;
    }
  }

  if (huart->Instance == USART2)
  {
    HAL_UART_Receive_IT(&huart2, &rxdata2, 1);
		switch(uart2_rx_state)
		{
			case 0:
				// 当检测到左大括号时，重置接收索引并开始接收数据
				if (rxdata2 == '{')
				{
					uart2_rx_state = 1;
					uart2_rx_i = 0; // 重置索引
					memset(uart2_rx_buff, 0, RXBUFF_MAX_SIZEOF); // 清空接收缓冲区
				}
				break;
			case 1:
				if(rxdata2 == '}')
				{
					// 确保最后一个字符正确终止字符串
					uart2_rx_buff[uart2_rx_i] = '\0';
					uart2_rx_state = 0;
					uart2_rx_flag = 1;
					// 保持向后兼容
					rx_flag = 1;
				}
				else if(uart2_rx_i < RXBUFF_MAX_SIZEOF - 1)
					uart2_rx_buff[uart2_rx_i++] = rxdata2;
				break;
		}
  }



}
