#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

typedef void *esp_websocket_client_handle_t;

enum {
    WEBSOCKET_EVENT_ANY = 0,
    WEBSOCKET_EVENT_CONNECTED,
    WEBSOCKET_EVENT_DISCONNECTED,
    WEBSOCKET_EVENT_DATA,
    WEBSOCKET_EVENT_ERROR,
    WS_TRANSPORT_OPCODES_TEXT = 0x01,
    WS_TRANSPORT_OPCODES_BINARY = 0x02,
};

typedef struct {
    esp_event_base_t event_base;
    int32_t event_id;
    void *event_data;
} esp_websocket_event_data_t;

typedef struct {
    const char *uri;
    void *user_context;
} esp_websocket_client_config_t;

typedef void (*esp_websocket_event_cb)(void *handler_args, esp_event_base_t base,
    int32_t event_id, void *event_data);

static inline esp_websocket_client_handle_t esp_websocket_client_init(
    const esp_websocket_client_config_t *config) {
    (void)config;
    return NULL;
}

static inline esp_err_t esp_websocket_client_start(esp_websocket_client_handle_t client) {
    (void)client;
    return ESP_FAIL;
}

static inline esp_err_t esp_websocket_client_stop(esp_websocket_client_handle_t client) {
    (void)client;
    return ESP_OK;
}

static inline void esp_websocket_client_destroy(esp_websocket_client_handle_t client) {
    (void)client;
}

static inline int esp_websocket_client_send_text(
    esp_websocket_client_handle_t client, const char *data, int len, int timeout_ms) {
    (void)client; (void)data; (void)len; (void)timeout_ms;
    return -1;
}

static inline int esp_websocket_client_send_bin(
    esp_websocket_client_handle_t client, const char *data, int len, int timeout_ms) {
    (void)client; (void)data; (void)len; (void)timeout_ms;
    return -1;
}

static inline esp_err_t esp_websocket_client_close(
    esp_websocket_client_handle_t client, int timeout_ms) {
    (void)client; (void)timeout_ms;
    return ESP_OK;
}

static inline void esp_websocket_register_events(
    esp_websocket_client_handle_t client, int event,
    esp_websocket_event_cb cb, void *ctx) {
    (void)client; (void)event; (void)cb; (void)ctx;
}

static inline bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client) {
    (void)client;
    return false;
}
