#ifndef __UART_H
#define __UART_H
#include "stm32f1xx_hal.h"

#define RXBUFF_MAX_SIZEOF      255

// USART1相关变量
extern uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart1_i;
extern uint8_t rxdata1;
extern uint8_t uart1_rx_buff[RXBUFF_MAX_SIZEOF];
extern uint8_t uart1_rx_state, uart1_rx_flag;
extern uint8_t uart1_rx_i;

// USART2相关变量（用于Zigbee）
extern uint8_t uart2_Buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart2_i;
extern uint8_t rxdata2;
extern uint8_t uart2_rx_buff[RXBUFF_MAX_SIZEOF];
extern uint8_t uart2_rx_state, uart2_rx_flag;
extern uint8_t uart2_rx_i;

// 全局变量（保持向后兼容）
extern uint8_t rx_buff[RXBUFF_MAX_SIZEOF], rx_state, rx_flag;
extern uint8_t rx_i;

#endif    /* __UART_H */


