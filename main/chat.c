#include "chat.h"

#include "http_server.h"

int g_clients[MAX_CLIENTS] = {0};
QueueHandle_t g_message_queue = NULL;
QueueHandle_t g_clients_queue = NULL;
SemaphoreHandle_t g_clients_mutex = NULL;

void init_chat(void) {
  g_message_queue = xQueueCreate(MAX_MESSAGE_QUEUE, sizeof(chat_message_t *));
  if (g_message_queue == NULL) {
    ESP_LOGE(WS_TAG, "Failed to create message queue");
    abort();
    return;
  }
  g_clients_queue = xQueueCreate(MAX_CLIENTS, sizeof(client_event_t));
  if (g_clients_queue == NULL) {
    ESP_LOGE(WS_TAG, "Failed to create clients queue");
    abort();
    return;
  }
  g_clients_mutex = xSemaphoreCreateMutex();
  if (g_clients_mutex == NULL) {
    ESP_LOGE(WS_TAG, "Failed to create clients mutex");
    abort();
    return;
  }

  xTaskCreate(manage_client_task, "manage_client_task", 4096, NULL, 7, NULL);
  xTaskCreate(message_broadcast_task, "message_broadcast_task", 4096, NULL, 6,
              NULL);
}

esp_err_t websocket_handler(httpd_req_t *req) {
  int sockfd = httpd_req_to_sockfd(req);
  httpd_ws_frame_t ws_frame;
  chat_message_t *message = NULL;

  if (req->method == HTTP_GET) {
    client_event_t event = {
        .sockfd = sockfd,
        .event = CLIENT_CONNECTED,
    };
    if (xQueueSend(g_clients_queue, &event, pdMS_TO_TICKS(1000)) != pdTRUE) {
      ESP_LOGW(WS_TAG, "Clients queue full");
      return ESP_OK;
    }
    ESP_LOGI(WS_TAG, "New WebSocket client connected");
    return ESP_OK;
  }

  memset(&ws_frame, 0, sizeof(httpd_ws_frame_t));
  ws_frame.type = HTTPD_WS_TYPE_TEXT;

  esp_err_t ret = httpd_ws_recv_frame(req, &ws_frame, 0);
  if (ret != ESP_OK || ws_frame.len == 0 || ws_frame.len >= MAX_MESSAGE_LEN) {
    ESP_LOGE(WS_TAG, "Failed to receive WebSocket frame");
    xSemaphoreTake(g_clients_mutex, portMAX_DELAY);
    remove_client(sockfd);
    xSemaphoreGive(g_clients_mutex);
    return ret;
  }

  ws_frame.payload = malloc(ws_frame.len + 1);
  if (ws_frame.payload == NULL) {
    ESP_LOGE(WS_TAG, "Failed to allocate memory for WebSocket frame payload");
    return ESP_ERR_NO_MEM;
  }

  ret = httpd_ws_recv_frame(req, &ws_frame, ws_frame.len);
  if (ret == ESP_OK) {
    ws_frame.payload[ws_frame.len] = '\0';
    ESP_LOGI(WS_TAG, "Received WebSocket message: %s",
             (char *)ws_frame.payload);

    message = malloc(sizeof(chat_message_t));
    if (message == NULL) {
      ESP_LOGE(WS_TAG, "Failed to allocate memory for chat message");
      free(ws_frame.payload);
      return ESP_ERR_NO_MEM;
    }

    message->sockfd = sockfd;
    message->len = ws_frame.len;
    message->message = (char *)ws_frame.payload;

    if (xQueueSend(g_message_queue, &message, pdMS_TO_TICKS(100)) != pdTRUE) {
      ESP_LOGW(WS_TAG, "Message queue full");
      free(ws_frame.payload);
      free(message);
    }
  } else {
    ESP_LOGE(WS_TAG, "Failed to receive WebSocket frame payload");
    free(ws_frame.payload);
  }
  return ret;
}

void message_broadcast_task(void *pvParameters) {
  chat_message_t *message = NULL;

  while (true) {
    if (xQueueReceive(g_message_queue, &message, portMAX_DELAY) == pdTRUE) {
      broadcast_message(message);
      free(message->message);
      free(message);
    }
  }
}

void broadcast_message(chat_message_t *message) {
  httpd_ws_frame_t ws_frame;
  ESP_LOGI(WS_TAG, "Broadcasting message: %s", message->message);

  memset(&ws_frame, 0, sizeof(httpd_ws_frame_t));
  ws_frame.type = HTTPD_WS_TYPE_TEXT;
  ws_frame.len = message->len;
  ws_frame.payload = (uint8_t *)message->message;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (g_clients[i] != 0 && g_clients[i] != message->sockfd) {
      esp_err_t ret =
          httpd_ws_send_frame_async(g_server, g_clients[i], &ws_frame);
      if (ret != ESP_OK) {
        ESP_LOGE(WS_TAG, "Failed to send WebSocket frame");
        xSemaphoreTake(g_clients_mutex, portMAX_DELAY);
        remove_client(g_clients[i]);
        xSemaphoreGive(g_clients_mutex);
      }
    }
  }
}

void ws_send_callback(int sockfd, esp_err_t status) {
  if (status != ESP_OK) {
    ESP_LOGE(WS_TAG, "Failed to send WebSocket frame");
    xSemaphoreTake(g_clients_mutex, portMAX_DELAY);
    remove_client(sockfd);
    xSemaphoreGive(g_clients_mutex);
  }
}

void manage_client_task(void *pvParameters) {
  client_event_t event = {0};

  while (true) {
    if (xQueueReceive(g_clients_queue, &event, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI(WS_TAG, "Client event: %d", event.event);
      xSemaphoreTake(g_clients_mutex, portMAX_DELAY);
      if (event.event == CLIENT_CONNECTED)
        add_client(event.sockfd);
      else
        remove_client(event.sockfd);
      xSemaphoreGive(g_clients_mutex);
    }
  }
}

void add_client(int sockfd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (g_clients[i] == 0) {
      g_clients[i] = sockfd;
      break;
    }
  }
}

void remove_client(int sockfd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (g_clients[i] == sockfd) {
      g_clients[i] = 0;
      break;
    }
  }
}