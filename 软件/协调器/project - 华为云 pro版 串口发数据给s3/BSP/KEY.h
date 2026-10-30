#ifndef __KEY_H 
#define __KEY_H
#include "stm32f1xx_hal.h"
//根据需求添加/减少按键
#define KEY_MAX_NUMBER    5
struct keys{
	uint8_t key_state;
	uint8_t key_flag;
	uint8_t key_value;
};
void KEY_Init(void);
void KEY_Scan(void);
#endif  /* __KEY_H */


