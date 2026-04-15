#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Channel identifiers */
#define MIMI_CHAN_TELEGRAM   "telegram"
#define MIMI_CHAN_FEISHU     "feishu"
#define MIMI_CHAN_WHATSAPP   "whatsapp"
#define MIMI_CHAN_WEBSOCKET  "websocket"
#define MIMI_CHAN_SLACK      "slack"
#define MIMI_CHAN_CLI        "cli"
#define MIMI_CHAN_SYSTEM     "system"

/* Message types on the bus */
typedef struct {
    char channel[16];       /* "telegram", "websocket", "cli" */
    char chat_id[96];       /* Telegram/Feishu chat_id, open_id, or WS client id */
    char *content;          /* Heap-allocated message text (caller must free) */
    
    /* F2: Extended fields for unified message schema */
    char user_id[64];      /* User identifier from channel */
    char message_id[64];    /* Message ID from channel */
    int media_count;        /* Number of media attachments (0-4) */
    char media_paths[4][64];/* Media file paths */
    char reply_to[64];      /* Message ID being replied to */
    char metadata[256];      /* Additional channel-specific data (JSON) */
} mimi_msg_t;

/**
 * Initialize the message bus (inbound + outbound FreeRTOS queues).
 */
esp_err_t message_bus_init(void);

/**
 * Push a message to the inbound queue (towards Agent Loop).
 * The bus takes ownership of msg->content.
 */
esp_err_t message_bus_push_inbound(const mimi_msg_t *msg);

/**
 * Pop a message from the inbound queue (blocking).
 * Caller must free msg->content when done.
 */
esp_err_t message_bus_pop_inbound(mimi_msg_t *msg, uint32_t timeout_ms);

/**
 * Push a message to the outbound queue (towards channels).
 * The bus takes ownership of msg->content.
 */
esp_err_t message_bus_push_outbound(const mimi_msg_t *msg);

/**
 * Pop a message from the outbound queue (blocking).
 * Caller must free msg->content when done.
 */
esp_err_t message_bus_pop_outbound(mimi_msg_t *msg, uint32_t timeout_ms);
