#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include "esp_log.h"

extern httpd_handle_t g_server;

void init_httpd(void);

static const char *SRV_LOG_TAG = "SERVER";
static const char *RESPONSE_404 = "404 Not Found";
static const char *RESPONSE_500 = "500 Internal Server Error";
static const ssize_t HTML_LEN = 1384;
static const char *RESPONSE_HTML =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>ESP32 WebSocket Chat</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; text-align: center; }"
    "#chatbox { width: 100%; height: 300px; border: 1px solid #ddd; "
    "overflow-y: scroll; padding: 10px; margin-bottom: 10px; }"
    "#message { width: 80%; padding: 10px; }"
    "</style>"
    "</head>"
    "<body>"
    "<h1>ESP32 WebSocket Chat</h1>"
    "<div id=\"chatbox\"></div>"
    "<input type=\"text\" id=\"message\" placeholder=\"Type a message...\">"
    "<button onclick=\"sendMessage()\">Send</button>"
    "<script>"
    "const chatbox = document.getElementById('chatbox');"
    "const ws = new WebSocket('ws://' + location.host + '/ws');"

    "ws.onopen = () => { chatbox.innerHTML += '<p><em>Connected to ESP32 "
    "WebSocket server</em></p>'; };"

    "ws.onmessage = (event) => {"
    "    const message = event.data;"
    "    chatbox.innerHTML += `<p><strong>ESP32:</strong> ${message}</p>`;"
    "    chatbox.scrollTop = chatbox.scrollHeight;"  // 자동 스크롤
    "};"

    "ws.onclose = () => { chatbox.innerHTML += '<p><em>Disconnected from "
    "server</em></p>'; };"

    "function sendMessage() {"
    "    const input = document.getElementById('message');"
    "    const message = input.value.trim();"
    "    if (message) {"
    "        chatbox.innerHTML += `<p><strong>You:</strong> ${message}</p>`;"
    "        ws.send(message);"
    "        input.value = '';"
    "        chatbox.scrollTop = chatbox.scrollHeight;"
    "    }"
    "}"
    "</script>"
    "</body>"
    "</html>";

#endif