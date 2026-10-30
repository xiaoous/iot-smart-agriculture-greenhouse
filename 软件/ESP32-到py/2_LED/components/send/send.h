#ifndef _SEND_H
#define _SEND_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_recv.h"
// 新增：HTTP服务端头文件
#include "esp_http_server.h"

// ------------------- 配置区（根据实际修改） -------------------
#define WIFI_SSID      "YOUR_WIFI_SSID"          // 你的WiFi名称（本地改成真实值，勿提交）
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"      // 你的WiFi密码（本地改成真实值，勿提交）
#define SERVER_URL     "http://192.168.1.100:5000/receive"  // Python服务器地址
#define HTTP_TIMEOUT_MS    5000      // HTTP超时时间
#define HTTP_RETRY_COUNT   3         // 重试次数
#define SEND_INTERVAL_MS   5000      // 发送间隔（5秒）
#define HTTP_TASK_STACK    4096 * 2  // 任务栈大小
// ---------------------------------------------------------

// 声明外部变量（从esp_recv.c获取STM32的真实数据）
extern int CO2_value, light, temp_value_2, kq_humi_value_2, t_humi_value;
extern float t_ph_value;

// ====================== 新增：全局指令变量 ======================
extern int g_pump_cmd;  // 水泵指令 0=关 1=开
extern int g_fog_cmd;   // 水雾指令 0=关 1=开

// HTTP事件回调
esp_err_t _http_event_handle(esp_http_client_event_t *evt);
// HTTP发送任务
void http_post_task(void *pvParameters);
// WiFi事件回调
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
// WiFi初始化
void wifi_init_sta(void);

// ====================== 新增：启动HTTP服务 ======================
void start_http_server(void);

#endif
