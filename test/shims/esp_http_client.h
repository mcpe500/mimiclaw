#pragma once
#include "esp_err.h"
#include <stddef.h>
#define ESP_HTTP_CLIENT_METHOD_GET     0
#define ESP_HTTP_CLIENT_METHOD_POST    1

typedef void *esp_http_client_handle_t;
typedef void *esp_http_client_config_t;

static inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config) { (void)config; return NULL; }
static inline esp_err_t esp_http_client_perform(esp_http_client_handle_t client) { (void)client; return 0; }
static inline int esp_http_client_get_status_code(esp_http_client_handle_t client) { (void)client; return 200; }
static inline void esp_http_client_cleanup(esp_http_client_handle_t client) { (void)client; }
