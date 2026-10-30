#include <stdio.h>
#include <string.h>
#include "esp_recv.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "Myuart.h"

// ========================= 【新增：队列配置】 =========================
#define UART1_SEND_QUEUE_LEN  10        // 队列最多缓存10条数据
#define UART1_SEND_DATA_SIZE  256       // 每条数据最大长度

QueueHandle_t uart1_send_queue = NULL;  // 发送队列句柄
// ======================================================================
void uart1_send_task(void *pvParameters);
void uart_recv_task(void *pvParameters);
// 定义错误返回值
#define RXERROR -1
// ESP32-S3 串口1接收缓存
uint8_t uart1_rx_buff[1024];
uint16_t uart1_rx_i = 0;
uint8_t uart1_rx_state = 0;
uint8_t uart1_rx_flag = 0;

// 全局变量
int CO2_value, light, temp_value_2, kq_humi_value_2, t_humi_value;
float t_ph_value;

// 功能：处理串口1接收到的完整STM32数据
void esp32_recv_process(void)
{
    if(uart1_rx_flag == 1)
    {
        // 解析数据
        temp_value_2 = esp32_parse_data("temp_value_2");
        kq_humi_value_2 = esp32_parse_data("kq_humi_value_2");
        t_humi_value = esp32_parse_data("t_humi_value");
        t_ph_value = esp32_parse_data("t_ph_value");
        CO2_value = esp32_parse_data("CO2_value");
        light = esp32_parse_data("light");

        // // 通过串口2发送从串口1接收到的原始数据
        // uart2_send_data((char *)uart1_rx_buff);
        // uart2_send_data("\r\n");

        // 处理完成后清除标志
        uart1_rx_flag = 0;
    }
}

 //将 解析 uart1_rx_buff数据缓冲区内数据，获取缓冲数据键值对的值 
 // 功能：从 uart1_rx_buff 里的 JSON 字符串，提取关键字对应的浮点数值
// 输入：keyword → 要找的字段名，如 "temp_value_2"、"CO2_value"
// 返回：成功返回数字，失败返回 -1
float esp32_parse_data(const char *keyword)
{
    char temp[10];
    uint8_t temp_i = 0;
    char *p1 = NULL;

    if ((p1 = strstr((char *)uart1_rx_buff, keyword)) != NULL)
    {
        p1 += strlen(keyword);
        while(*p1 != ':' && *p1 != '\0') p1++;
        if(*p1 == '\0') return -1;
        p1++;
        while(*p1 != ',' && *p1 != '}' && *p1 != '\0' && temp_i < 9)
        {
            temp[temp_i++] = *p1++;
        }
        temp[temp_i] = '\0';
        return atof(temp);
    }
    return -1;
}

// 初始化
void esp_recv_init(void)
{
    uart_recv_start();
}


// 启动串口接收（任务方式） + 初始化队列
void uart_recv_start(void)
{
    // ===================== 【新增：创建队列 + 发送任务】 =====================
    uart1_send_queue = xQueueCreate(UART1_SEND_QUEUE_LEN, UART1_SEND_DATA_SIZE);
    xTaskCreate(uart1_send_task, "uart1_send_task", 4096, NULL, 4, NULL);
    // ========================================================================



    // 接收任务
    xTaskCreate(uart_recv_task, "uart_recv_task", 4096, NULL, 5, NULL);
}

// 接收任务(串口接收任务，接收stm32发来的数据）
void uart_recv_task(void *pvParameters)
{
    uint8_t data;
    while(1)
    {
        int len = uart_read_bytes(UART_NUM_1, &data, 1, 100 / portTICK_PERIOD_MS);
        if(len == 1)
        {
            switch(uart1_rx_state)
            {
                case 0:
                    if (data == '{')
                    {
                        uart1_rx_state = 1;
                        uart1_rx_i = 0;
                        memset(uart1_rx_buff, 0, sizeof(uart1_rx_buff));
                    }
                    break;
                case 1:
                    if(data == '}')
                    {
                        uart1_rx_buff[uart1_rx_i] = '\0';
                        uart1_rx_state = 0;
                        uart1_rx_flag = 1;
                    }
                    else if(uart1_rx_i < sizeof(uart1_rx_buff) - 1)
                        uart1_rx_buff[uart1_rx_i++] = data;
                    break;
            }
        }
    }
}

// ========================= 【 读队列UART1 发送任务】 =========================
void uart1_send_task(void *pvParameters)
{
    char buffer[UART1_SEND_DATA_SIZE];

    while (1)
    {
        // 从队列取数据（阻塞等待）
        if (xQueueReceive(uart1_send_queue, buffer, portMAX_DELAY))
        {
            // 真正发送到 STM32（UART1）
            uart_write_bytes(UART_NUM_1, buffer, strlen(buffer));

            // 日志
            // uart2_send_data("[UART1-QUEUE] Send to STM32: ");
            // uart2_send_data(buffer);
            // uart2_send_data("\r\n");
        }
    }
}

// ========================= 【 将发送数据写队列 】 =========================
void uart1_send_to_stm32(const char *data)
{
    if (data == NULL || uart1_send_queue == NULL) return;

    char buffer[UART1_SEND_DATA_SIZE];
    strncpy(buffer, data, UART1_SEND_DATA_SIZE - 1);
    buffer[UART1_SEND_DATA_SIZE - 1] = 0;

    // 入队（不阻塞）
    xQueueSend(uart1_send_queue, buffer, 0);
}