#include "wifi.h"

#include "http_server.h"

QueueHandle_t g_wifi_event_queue = NULL;
TaskHandle_t g_wifi_event_task_handle = NULL;

static uint_fast8_t retry_count = 0;
static const uint_fast8_t MAX_RETRY = 5;

void wifi_event_task(void* pvParameters) {
  ft_wifi_event_t event;

  while (true) {
    if (xQueueReceive(g_wifi_event_queue, &event, portMAX_DELAY)) {
      switch (event) {
        case WIFI_CONNECTED:
          ESP_LOGI(WIFI_TAG, "Connected to Wi-Fi");
          retry_count = 0;
          init_httpd();
          break;
        case WIFI_DISCONNECTED:
          if (retry_count < MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGW(WIFI_TAG, "Disconnected from Wi-Fi retrying... (%d/%d)",
                     retry_count, MAX_RETRY);
          } else {
            ESP_LOGE(WIFI_TAG, "Failed to connect to Wi-Fi after %d attempts",
                     MAX_RETRY);
            graceful_shutdown();
          }
          break;
        default:
          ESP_LOGE(WIFI_TAG, "Unknown Wi-Fi event should not be here");
          graceful_shutdown();
          break;
      }
    }
  }

  vTaskDelete(NULL);
}

void init_wifi_task(void) {
  esp_netif_init();
  esp_event_loop_create_default();

  esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
  esp_netif_ip_info_t ip_info;

  ip_info.ip.addr = inet_addr(CONFIG_WIFI_LOCAL_IP);
  ip_info.gw.addr = inet_addr(CONFIG_WIFI_GATEWAY);
  ip_info.netmask.addr = inet_addr(CONFIG_WIFI_SUBNET);

  esp_netif_dhcpc_stop(sta_netif);
  esp_netif_set_ip_info(sta_netif, &ip_info);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                             NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                             NULL);

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_WIFI_SSID,
              .password = CONFIG_WIFI_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();

  g_wifi_event_queue = xQueueCreate(5, sizeof(ft_wifi_event_t));
  if (g_wifi_event_queue == NULL) {
    ESP_LOGE(WIFI_TAG, "Failed to create wifi_event_queue");
    abort();
    return;
  }
  xTaskCreate(wifi_event_task, "wifi_event_task", 4096, NULL, 8,
              &g_wifi_event_task_handle);
}

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
  ft_wifi_event_t event;
  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      event = WIFI_DISCONNECTED;
      xQueueSend(g_wifi_event_queue, &event, portMAX_DELAY);
    } else {
      ESP_LOGI(WIFI_TAG, "Wi-Fi event: %ld", event_id);
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* ip_event = (ip_event_got_ip_t*)event_data;
      ESP_LOGI(WIFI_TAG, "Got IP: " IPSTR, IP2STR(&ip_event->ip_info.ip));

      event = WIFI_CONNECTED;
      xQueueSend(g_wifi_event_queue, &event,
                 portMAX_DELAY);  // IP 할당 완료 시 연결 성공 이벤트 전송
    }
  }
}

void graceful_shutdown(void) {
  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               &wifi_event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                               &wifi_event_handler);
  if (g_server) {
    httpd_stop(g_server);
  }
  if (g_wifi_event_task_handle != NULL) {
    vTaskDelete(g_wifi_event_task_handle);
  }
  if (g_wifi_event_queue != NULL) {
    vQueueDelete(g_wifi_event_queue);
  }
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_netif_deinit();
}