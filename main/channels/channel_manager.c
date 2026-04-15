#include "channel.h"
#include "message_bus.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "channel_mgr";

#define MAX_CHANNELS 8

static channel_t *g_channels[MAX_CHANNELS];
static int g_channel_count = 0;

esp_err_t channel_register(const char *id, channel_create_fn create_fn)
{
    if (g_channel_count >= MAX_CHANNELS) {
        ESP_LOGE(TAG, "Max channels reached");
        return ESP_ERR_NO_MEM;
    }

    channel_t *chan = create_fn();
    if (!chan) return ESP_FAIL;

    strncpy(chan->channel_id, id, sizeof(chan->channel_id) - 1);
    g_channels[g_channel_count++] = chan;

    ESP_LOGI(TAG, "Registered channel: %s", id);
    return ESP_OK;
}

channel_t *channel_get(const char *id)
{
    for (int i = 0; i < g_channel_count; i++) {
        if (strcmp(g_channels[i]->channel_id, id) == 0) {
            return g_channels[i];
        }
    }
    return NULL;
}

esp_err_t channel_init_all(void)
{
    esp_err_t err = ESP_OK;

    /* Register standard channels */
    channel_register("telegram", channel_telegram_create);
    channel_register("feishu", channel_feishu_create);
    channel_register("websocket", channel_websocket_create);

    /* Connect all channels */
    for (int i = 0; i < g_channel_count; i++) {
        channel_t *chan = g_channels[i];
        if (chan->vtable->connect) {
            esp_err_t cerr = chan->vtable->connect(chan);
            if (cerr != ESP_OK) {
                ESP_LOGW(TAG, "Channel %s failed to connect: %d", chan->channel_id, cerr);
                err = cerr;
            }
        }
    }

    return err;
}

esp_err_t channel_shutdown_all(void)
{
    for (int i = 0; i < g_channel_count; i++) {
        channel_t *chan = g_channels[i];
        if (chan->vtable->disconnect) {
            chan->vtable->disconnect(chan);
        }
        if (chan->vtable->free) {
            chan->vtable->free(chan);
        }
        free(chan);
    }
    g_channel_count = 0;
    return ESP_OK;
}

/* Stub implementations for standard channels */
channel_t *channel_telegram_create(void)
{
    channel_t *chan = calloc(1, sizeof(channel_t));
    if (!chan) return NULL;
    strncpy(chan->name, "telegram", sizeof(chan->name) - 1);
    chan->caps.supports_pairing = true;
    chan->caps.supports_groups = true;
    chan->caps.supports_media = true;
    chan->caps.supports_inline_media = false;
    return chan;
}

channel_t *channel_feishu_create(void)
{
    channel_t *chan = calloc(1, sizeof(channel_t));
    if (!chan) return NULL;
    strncpy(chan->name, "feishu", sizeof(chan->name) - 1);
    chan->caps.supports_pairing = false;
    chan->caps.supports_groups = true;
    chan->caps.supports_media = false;
    chan->caps.supports_inline_media = false;
    return chan;
}

channel_t *channel_websocket_create(void)
{
    channel_t *chan = calloc(1, sizeof(channel_t));
    if (!chan) return NULL;
    strncpy(chan->name, "websocket", sizeof(chan->name) - 1);
    chan->caps.supports_pairing = false;
    chan->caps.supports_groups = false;
    chan->caps.supports_media = false;
    chan->caps.supports_inline_media = false;
    return chan;
}