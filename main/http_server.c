#include "http_server.h"

#include "chat.h"

httpd_handle_t g_server = NULL;

// HTTP GET 요청 핸들러
static esp_err_t html_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, RESPONSE_HTML, HTML_LEN);
  return ESP_OK;
}

// HTTP 및 웹소켓 서버 태스크
void init_httpd(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.task_priority = 5;
  // config.uri_match_fn = httpd_uri_match_wildcard;

  httpd_uri_t html_uri = {.uri = "/",
                          .method = HTTP_GET,
                          .handler = html_handler,
                          .user_ctx = NULL};
  httpd_uri_t ws_uri = {.uri = "/ws",
                        .method = HTTP_GET,
                        .handler = websocket_handler,
                        .user_ctx = NULL,
                        .is_websocket = true};

  if (httpd_start(&g_server, &config) == ESP_OK) {
    init_chat();
    httpd_register_uri_handler(g_server, &html_uri);
    httpd_register_uri_handler(g_server, &ws_uri);
    ESP_LOGI(SRV_LOG_TAG, "HTTP and WebSocket server started");
  } else {
    ESP_LOGE(SRV_LOG_TAG, "Failed to start HTTP server");
  }
}
