#ifndef __ZIGBEE_H
#define __ZIGBEE_H

#include "stm32f1xx_hal.h"
#include "delay.h"
#include "OLED.h"

#define REV_WAIT 2
#define REV_OK 1
#define RXERROR -1
void Zigbee_Init(void);
void Zigbee_SendString(char *s);
void Zigbee_Recv_Data(void);
#endif
