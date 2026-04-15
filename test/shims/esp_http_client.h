#pragma once
#include "esp_err.h"
#include <stddef.h>

#define ESP_HTTP_CLIENT_METHOD_GET     0
#define ESP_HTTP_CLIENT_METHOD_POST    1
#define HTTP_METHOD_HEAD               2
#define HTTP_METHOD_POST               3

typedef void *esp_http_client_handle_t;

typedef struct {
    const char *url;
    int method;
    int timeout_ms;
    void *crt_bundle_attach;
    esp_err_t (*event_handler)(struct esp_http_client_event_t *evt);
    void *user_data;
    int buffer_size;
} esp_http_client_config_t;

typedef struct esp_http_client_event_t {
    int event_id;
    void *user_data;
    const char *header_key;
    const char *header_value;
    size_t data_len;
    const char *data;
} esp_http_client_event_t;

#define HTTP_EVENT_ON_HEADER 1
#define HTTP_EVENT_ON_DATA 2

static inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config) { (void)config; return (esp_http_client_handle_t)1; }
static inline esp_err_t esp_http_client_perform(esp_http_client_handle_t client) { (void)client; return ESP_OK; }
static inline int esp_http_client_get_status_code(esp_http_client_handle_t client) { (void)client; return 200; }
static inline void esp_http_client_cleanup(esp_http_client_handle_t client) { (void)client; }
static inline esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char *name, const char *value) { (void)client; (void)name; (void)value; return ESP_OK; }
static inline esp_err_t esp_http_client_set_method(esp_http_client_handle_t client, int method) { (void)client; (void)method; return ESP_OK; }
static inline esp_err_t esp_http_client_set_post_field(esp_http_client_handle_t client, const char *field, size_t len) { (void)client; (void)field; (void)len; return ESP_OK; }
