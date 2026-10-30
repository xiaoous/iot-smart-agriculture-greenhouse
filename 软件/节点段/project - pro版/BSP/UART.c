#include "UART.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart1_i;
uint8_t rxdata1;

uint8_t uart2_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart2_i;
uint8_t rxdata2;


uint8_t uart3_Buff[6];
uint16_t uart3_i;
uint8_t rxdata3;
uint8_t rx3_flag, rx3_state;


uint8_t rx_buff[RXBUFF_MAX_SIZEOF], rx_state, rx_flag;
uint8_t rx_i;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart1_Buff[uart1_i++] = rxdata1;
    HAL_UART_Receive_IT(&huart1, &rxdata1, 1);
		switch(rx_state)
		{
			case 0:
				if (rxdata1 == 'M')
					rx_state = 1;
				break;
			case 1:
				if (rxdata1 == 'Q')
					rx_state = 2;
				else
					rx_state = 0;
				break;
			case 2:
				if (rxdata1 == 'T')
					rx_state = 3;
				else
					rx_state = 0;
				break;
			case 3: 
				if (rxdata1 == 'T')
					rx_state = 4;
				else
					rx_state = 0;
				break;
			case 4:
				if (rxdata1 == 'S')
					rx_state = 5;
				else
					rx_state = 0;
				break;
			case 5:
				if (rxdata1 == 'U')
					rx_state = 6;
				else
					rx_state = 0;
			break;
			case 6:
				if (rxdata1 == 'B')
					rx_state = 7;
				else
					rx_state = 0;
				break;
			case 7:
				if (rxdata1 == 'R')
					rx_state = 8;
				else
					rx_state = 0;
			break;
			case 8:
				if (rxdata1 == 'E')
					rx_state = 9;
				else
					rx_state = 0;
			break;
			case 9:
				if (rxdata1 == 'C')
					rx_state = 10;
				else
					rx_state = 0;
			break;
			case 10:
				if (rxdata1 == 'V')
					rx_state = 11;
				else
					rx_state = 0;
			break;
			case 11:
				if(rxdata1 == '}')
				{
					rx_state = 0;
					rx_flag = 1;
					rx_i = 0;
				}
				else
					rx_buff[rx_i++] = rxdata1;
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
		switch(rx_state)
		{
			case 0:
				if (rxdata2 == '{')
					rx_state = 1;
				break;
			case 1:
				if(rxdata2 == '}')
				{
					rx_state 	= 0;
					rx_flag 	= 1;
					rx_i 			= 0;
				}
				else
					rx_buff[rx_i++] = rxdata2;
				break;
		}
  }

	  if (huart->Instance == USART3)
    {
      HAL_UART_Receive_IT(&huart3, &rxdata3, 1);
        switch(rx3_state)
        {
            case 0:
                uart3_i = 0;
                // 收到起始字符0x2C时，将其存入缓冲区并进入状态1
                if (rxdata3 == 0x2C)
                {
                    uart3_Buff[uart3_i++] = rxdata3; // 保存起始字符
                    rx3_state = 1;
                }
                break;
            case 1:
                // 继续接收剩余5个数据字节，总共6个字节
                if (uart3_i < 6)
                {
                    uart3_Buff[uart3_i++] = rxdata3;
                }
                if (uart3_i == 6) // 收到完整的6个字节
                {
                    rx3_flag = 1;
                    rx3_state = 0;
                }
                break;
        }
    }

}

