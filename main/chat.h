#ifndef CHAT_H
#define CHAT_H

#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LEN 128
#define MAX_MESSAGE_QUEUE 10
#define WS_TAG "WebSocket"

extern int g_clients[MAX_CLIENTS];
extern QueueHandle_t g_message_queue;
extern QueueHandle_t g_clients_queue;
extern SemaphoreHandle_t g_clients_mutex;

typedef struct {
  int sockfd;
  int len;
  char* message;
} chat_message_t;

typedef struct {
  int sockfd;
  enum { CLIENT_CONNECTED, CLIENT_DISCONNECTED } event;
} client_event_t;

void init_chat(void);
esp_err_t websocket_handler(httpd_req_t* req);
void message_broadcast_task(void* pvParameters);
void manage_client_task(void* pvParameters);

void ws_send_callback(int sockfd, esp_err_t status);
void add_client(int sockfd);
void remove_client(int sockfd);
void broadcast_message(chat_message_t* message);

#endif