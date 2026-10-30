#ifndef __ESP_RECV_H__
#define __ESP_RECV_H__


void uart1_send_to_stm32(const char *data);
float esp32_parse_data(const char *keyword);
void esp32_recv_process(void);
void esp_recv_init(void);
void uart_recv_start(void);
void test_cloud_data_processing(void);
void process_cloud_data(const char *cloud_data);
int16_t Received_Data_Processing(const char *str, uint8_t len, uint8_t *buffer);
#endif
