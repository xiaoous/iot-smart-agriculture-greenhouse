#ifndef __ESP32S3_H
#define __ESP32S3_H

#include "main.h"

// 避免与zigbee.h中的宏定义冲突
#define ESP32S3_REV_OK 		0
#define ESP32S3_REV_WAIT 	1
#define ESP32S3_RXERROR 	0xFF

void ESP32S3_Init(void);
void ESP32S3_SendData(char *str);
int8_t ESP32S3_GetIPD(unsigned short timeOut);
void ESP32S3_Clear(void);

#endif

