#ifndef __DHT11_H
#define __DHT11_H
#include "main.h"
#include "stdio.h"
#define DHT11_GPIO_PIN					GPIO_PIN_7
#define DHT11_GPIO							GPIOB
#define DHT11_OutPut(x)					do{x ?	\
																		HAL_GPIO_WritePin(DHT11_GPIO, DHT11_GPIO_PIN, GPIO_PIN_SET) :	\
																		HAL_GPIO_WritePin(DHT11_GPIO, DHT11_GPIO_PIN, GPIO_PIN_RESET);	\
																}while(0)
#define DHT11_Input							HAL_GPIO_ReadPin(DHT11_GPIO, DHT11_GPIO_PIN)						
void DHT11_Init(void);
void DHT11_Read_Data(uint8_t *humi, uint8_t *temp);
#endif
