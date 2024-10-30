#ifndef WIFI_H
#define WIFI_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

typedef enum { WIFI_CONNECTED, WIFI_DISCONNECTED } ft_wifi_event_t;

extern QueueHandle_t g_wifi_event_queue;
extern TaskHandle_t g_wifi_event_task_handle;
static const char* WIFI_TAG = "WiFi_Event";

void init_wifi_task(void);
void wifi_event_task(void* pvParameters);
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data);
void graceful_shutdown(void);

#endif