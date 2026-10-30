#include <stdio.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_recv.h"
#include <string.h>
// 串口2发送函数
// void uart2_send_data(const char *data)
// {
//     uart_write_bytes(UART_NUM_2, data, strlen(data));
// }
// void uart1_send_data(const char *data)
// {
//     uart_write_bytes(UART_NUM_1, data, strlen(data));
// }
void uart_init(void)
{
    // 串口1配置（接收数据）
    uart_config_t uart_structure = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 100,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };
    uart_param_config(UART_NUM_1, &uart_structure);
    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, -1, -1); 
    uart_driver_install(UART_NUM_1, 1024, 1024, 10, NULL, 0);  // 不使用队列
    
    // // 串口2配置（发送数据）
    // uart_config_t uart2_structure = {
    //     .baud_rate = 115200,
    //     .data_bits = UART_DATA_8_BITS,
    //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //     .parity = UART_PARITY_DISABLE,
    //     .rx_flow_ctrl_thresh = 100,
    //     .source_clk = UART_SCLK_DEFAULT,
    //     .stop_bits = UART_STOP_BITS_1,
    // };
    // uart_param_config(UART_NUM_2, &uart2_structure);
    // uart_set_pin(UART_NUM_2, GPIO_NUM_19, GPIO_NUM_20, -1, -1);
    // uart_driver_install(UART_NUM_2, 1024, 0, 0, NULL, 0);
    
    // 启动串口1接收
    // uart_recv_start();
}
