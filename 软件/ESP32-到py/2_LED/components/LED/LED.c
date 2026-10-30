#include <stdio.h>
#include "LED.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
void func(void)
{
    
    gpio_config_t gpio_cfg= {0};
    gpio_cfg.intr_type=GPIO_INTR_DISABLE;
    gpio_cfg.mode=GPIO_MODE_INPUT_OUTPUT;
    gpio_cfg.pin_bit_mask=1ull<<GPIO_NUM_38;
    gpio_cfg.pull_down_en=GPIO_PULLDOWN_DISABLE;
    gpio_cfg.pull_up_en=GPIO_PULLUP_ENABLE; 
    gpio_config(&gpio_cfg);
}
