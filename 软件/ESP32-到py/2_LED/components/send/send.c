#include <stdio.h>
#include "send.h"
#include "cJSON.h"
static const char *TAG = "HTTP_CLIENT";
static TaskHandle_t http_post_task_handle = NULL;
static esp_http_client_handle_t http_client = NULL;

static void http_client_deinit(void);
static esp_err_t send_post_request(void);
// ===================== 新增：全局指令变量（发给STM32用） =====================
int g_pump_cmd = 0;    // 水泵 1=开 0=关
int g_fog_cmd = 0;     // 水雾 1=开 0=关

// ===================== 接收python数据解析------>stm32 =====================

// ===================== HTTP接收指令处理函数 =====================
esp_err_t control_handler(httpd_req_t *req)
{
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) { httpd_resp_send_500(req); return ESP_FAIL; }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "✅ Python JSON：%s", buf);
    cJSON *root = cJSON_Parse(buf);

    // 安全判断：解析失败直接返回
    if (root == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char send_buffer[256] = {0};
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");

    if (cmd == NULL) {
        httpd_resp_send_500(req);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // ====================== 设备控制（模式6） ======================
    if (strcmp(cmd->valuestring, "control") == 0) {
        int pump = cJSON_GetObjectItem(root, "pump")->valueint;
        int fog = cJSON_GetObjectItem(root, "fog")->valueint;

        snprintf(send_buffer, sizeof(send_buffer),
                 "{\"mode\":6,\"water_state_2\":%d,\"pump_state\":%d}\r\n",
                  fog, pump);
    }
    // ====================== 阈值设置（模式5 → 完整解析） ======================
    else if (strcmp(cmd->valuestring, "threshold") == 0) {
        // 解析所有6个参数（全部解析，不缺任何一个）
        int temp_max = 0;
        int humi_min = 0;
        int soil_humi_min = 0;
        int ph_max = 0;
        int light_min = 0;
        int CO2_max = 0;

        // 安全获取每个字段（不存在不会崩溃）
        cJSON *item;
        if ((item = cJSON_GetObjectItem(root, "temp_max")) != NULL) temp_max = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "humi_min")) != NULL) humi_min = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "soil_humi_min")) != NULL) soil_humi_min = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "ph_max")) != NULL) ph_max = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "light_min")) != NULL) light_min = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "CO2_max")) != NULL) CO2_max = item->valueint;

        // 完整发送所有参数（你App里的参数全部包含）
        snprintf(send_buffer, sizeof(send_buffer),
                 "{\"mode\":5,\"temp_max_2\":%d,\"kq_humi_min_2\":%d,\"t_humi_min\":%d,\"t_ph_max\":%d,\"light_min\":%d,\"CO2_max\":%d}\r\n",
                 temp_max,
                 humi_min,
                 soil_humi_min,
                 ph_max,
                 light_min,
                 CO2_max);
    }

    // 发送到 STM32（队列安全发送）
    uart1_send_to_stm32(send_buffer);
    ESP_LOGI(TAG, "✅ 发送给STM32：%s", send_buffer);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}
// ===================== 启动HTTP接收服务 =====================
void start_http_receive_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_uri_t control_uri = {
        .uri      = "/control",
        .method   = HTTP_POST,
        .handler  = control_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &control_uri);
        ESP_LOGI(TAG, "✅ HTTP接收服务启动，监听 /control");
    }
}


// ===================== 接收stm32数据---->python =====================

//初始化wifi底层，开启WiFi功能
void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi初始化完成");
}

// ===================== wifi回调函数启动接收服务 =====================
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi断开，重连中...");
        http_client_deinit();
        http_post_task_handle = NULL;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi连接成功，IP地址：" IPSTR, IP2STR(&event->ip_info.ip));

        // 👇👇👇 只加了这一行 👇👇👇
        start_http_receive_server();

        if (http_post_task_handle == NULL) {
            BaseType_t ret = xTaskCreate(http_post_task, "http_post_task", 
                                        HTTP_TASK_STACK, NULL, 5, &http_post_task_handle);
            if (ret != pdPASS) {
                ESP_LOGE(TAG, "创建HTTP发送任务失败");
                http_post_task_handle = NULL;
            } else {
                ESP_LOGI(TAG, "HTTP发送任务创建成功");
            }
        }
    }
}

//清除客户端
static void http_client_deinit(void)
{
    if (http_client != NULL) {
        esp_http_client_cleanup(http_client);
        http_client = NULL;
    }
}

//创建http客户端
static esp_err_t http_client_init_reuse(void)
{
    if (http_client != NULL) {
        return ESP_OK;
    }

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = _http_event_handle,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .disable_auto_redirect = true,
    };

    http_client = esp_http_client_init(&config);
    if (http_client == NULL) {
        ESP_LOGE(TAG, "HTTP客户端初始化失败");
        return ESP_FAIL;
    }

    return ESP_OK;
}

//客户端回调函数    连接、收数据、出错时自动做什么
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGI(TAG, "服务器返回：%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP请求发生错误");
            break;
        default:
            break;
    }
    return ESP_OK;
}

//创建http发送函数    esp32---> python 
void http_post_task(void *pvParameters)
{
    if (http_client_init_reuse() != ESP_OK) {
        ESP_LOGE(TAG, "HTTP客户端初始化失败，任务退出");
        http_post_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    while(1)
    {
        send_post_request();
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}

//将接收到stm32的数据通过http发送到py
static esp_err_t send_post_request(void)
{
    if (http_client_init_reuse() != ESP_OK) {
        return ESP_FAIL;
    }

    char post_data[256] = {0};
    snprintf(post_data, sizeof(post_data), 
             "{\"temp_value_2\":%d,\"kq_humi_value_2\":%d,\"t_humi_value\":%d,\"t_ph_value\":%.2f,\"CO2_value\":%d,\"light\":%d}",
             temp_value_2, kq_humi_value_2, t_humi_value, t_ph_value, CO2_value, light);

    esp_http_client_set_method(http_client, HTTP_METHOD_POST);
    esp_http_client_set_header(http_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(http_client, post_data, strlen(post_data));

    esp_err_t err;
    for (int retry = 0; retry < HTTP_RETRY_COUNT; retry++) {
        err = esp_http_client_perform(http_client);
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(http_client);
            if (status_code >= 200 && status_code < 300) {
                ESP_LOGI(TAG, "发送成功，数据：%s | HTTP状态码：%d", post_data, status_code);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "服务器返回异常状态码：%d，第%d次重试", status_code, retry + 1);
            }
        } else {
            ESP_LOGE(TAG, "发送失败（%s），第%d次重试", esp_err_to_name(err), retry + 1);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG, "重试%d次后仍失败，放弃本次发送", HTTP_RETRY_COUNT);
    return err;
}