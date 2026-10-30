#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "LED.h"
#include "Myuart.h"
#include "esp_recv.h"
#include "send.h"  

// 【必须加】外部声明：WiFi 初始化函数（在 send.c 里）
void wifi_init_sta(void);

void app_main(void)
{
    

    // 2. 初始化串口1和串口2
    uart_init();
    
    // 3. 初始化串口接收（任务+队列方式）
    esp_recv_init();        // 包含了 UART1 接收任务 + 发送队列任务

    // 4. 初始化WiFi（send.c 里的功能：上传数据 + 接收Python指令）
    wifi_init_sta();        // 启动 HTTP 服务 + 定时上传数据
    
    // 5. 主循环：只处理解析 STM32 发来的数据
    while (1)
    {
        esp32_recv_process();       // 解析串口1收到的STM32数据
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延时改小一点，响应更快
    }
}