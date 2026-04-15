#pragma once
#include "esp_err.h"
#include <stddef.h>

#define ESP_HTTP_CLIENT_METHOD_GET     0
#define ESP_HTTP_CLIENT_METHOD_POST    1

typedef void *esp_http_client_handle_t;

typedef struct {
    const char *url;
    int method;
} esp_http_client_config_t;

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_perform(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
void esp_http_client_cleanup(esp_http_client_handle_t client);
