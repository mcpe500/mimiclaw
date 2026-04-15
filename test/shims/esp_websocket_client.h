#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

typedef void *esp_websocket_client_handle_t;
typedef void *esp_websocket_event_handle_t;

typedef enum {
    WEBSOCKET_EVENT_ANY = 0,
    WEBSOCKET_EVENT_CONNECTED,
    WEBSOCKET_EVENT_DISCONNECTED,
    WEBSOCKET_EVENT_DATA,
    WEBSOCKET_EVENT_ERROR,
    WEBSOCKET_EVENT_CLOSED,
    WEBSOCKET_EVENT_BEFORE_CONNECT,
} esp_websocket_event_id_t;

typedef struct {
    const char *data_ptr;
    int data_len;
    uint8_t op_code;
    int payload_offset;
    int payload_len;
} esp_websocket_event_data_t;

typedef void (*esp_websocket_event_handler_t)(void *handler_args, esp_websocket_event_handle_t client, esp_websocket_event_data_t *data);

typedef void (*esp_websocket_crt_bundle_attach_t)(void);

typedef struct {
    const char *uri;
    esp_websocket_crt_bundle_attach_t crt_bundle_attach;
    int buffer_size;
    int buffer_size_tx;
    int reconnect_timeout_ms;
    int network_timeout_ms;
    int task_stack;
    bool disable_auto_reconnect;
    void *user_context;
} esp_websocket_client_config_t;

static inline esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *cfg) { (void)cfg; return NULL; }
static inline esp_err_t esp_websocket_client_start(esp_websocket_client_handle_t client) { (void)client; return 0; }
static inline esp_err_t esp_websocket_client_stop(esp_websocket_client_handle_t client) { (void)client; return 0; }
static inline esp_err_t esp_websocket_client_destroy(esp_websocket_client_handle_t client) { (void)client; return 0; }
static inline int esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout) { (void)client; (void)data; (void)len; (void)timeout; return 0; }
static inline int esp_websocket_client_send_bin(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout) { (void)client; (void)data; (void)len; (void)timeout; return 0; }
static inline bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client) { (void)client; return false; }
static inline esp_err_t esp_websocket_client_close(esp_websocket_client_handle_t client, TickType_t timeout) { (void)client; (void)timeout; return 0; }
static inline esp_err_t esp_websocket_register_events(esp_websocket_client_handle_t client, esp_websocket_event_id_t event, esp_websocket_event_handler_t handler, void *handler_args) { (void)client; (void)event; (void)handler; (void)handler_args; return 0; }
