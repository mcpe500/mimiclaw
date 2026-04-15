#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Channel capabilities */
typedef struct {
    bool supports_pairing;
    bool supports_groups;
    bool supports_media;
    bool supports_inline_media;
} channel_caps_t;

/* Channel interface (virtual table) */
typedef struct channel channel_t;

typedef esp_err_t (*channel_connect_fn)(channel_t *chan);
typedef esp_err_t (*channel_disconnect_fn)(channel_t *chan);
typedef esp_err_t (*channel_poll_fn)(channel_t *chan);
typedef esp_err_t (*channel_send_fn)(channel_t *chan, const char *chat_id, const char *text);

typedef struct {
    channel_connect_fn connect;
    channel_disconnect_fn disconnect;
    channel_poll_fn poll;
    channel_send_fn send;
    esp_err_t (*free)(channel_t *chan);
} channel_vtable_t;

/* Channel instance */
struct channel {
    const channel_vtable_t *vtable;
    char channel_id[16];
    char name[32];
    channel_caps_t caps;
    void *context;
};

/* Channel registry */
typedef channel_t* (*channel_create_fn)(void);

esp_err_t channel_register(const char *id, channel_create_fn create_fn);
channel_t *channel_get(const char *id);
esp_err_t channel_init_all(void);
esp_err_t channel_shutdown_all(void);

/* Helper */
#define CHANNEL_CALL(chan, method, ...) \
    ((chan) && (chan)->vtable->method ? (chan)->vtable->method((chan), ##__VA_ARGS__) : ESP_ERR_INVALID_ARG)

/* Standard channels */
channel_t *channel_telegram_create(void);
channel_t *channel_feishu_create(void);
channel_t *channel_websocket_create(void);

#ifdef __cplusplus
}
#endif