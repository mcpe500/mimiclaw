#include "discord_bot.h"
#include "mimi_config.h"
#include "bus/message_bus.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_websocket_client.h"

/* ── Config fallbacks ────────────────────────────────────────── */

#ifndef MIMI_CHAN_DISCORD
#define MIMI_CHAN_DISCORD             "discord"
#endif

#ifndef MIMI_NVS_DISCORD
#define MIMI_NVS_DISCORD              "discord_config"
#endif

#ifndef MIMI_SECRET_DISCORD_TOKEN
#define MIMI_SECRET_DISCORD_TOKEN     ""
#endif

#ifndef MIMI_DISCORD_GW_STACK
#define MIMI_DISCORD_GW_STACK         (12 * 1024)
#endif

#ifndef MIMI_DISCORD_GW_PRIO
#define MIMI_DISCORD_GW_PRIO          5
#endif

#ifndef MIMI_DISCORD_MAX_MSG_LEN
#define MIMI_DISCORD_MAX_MSG_LEN      2000
#endif

#define DISCORD_NVS_KEY_TOKEN         "bot_token"
#define DISCORD_DEDUP_CACHE_SIZE      64
#define DISCORD_GATEWAY_URL           "wss://gateway.discord.gg/?v=10&encoding=json"
#define DISCORD_API_BASE              "https://discord.com/api/v10"

/* Discord Gateway opcodes */
#define DISCORD_OP_DISPATCH           0
#define DISCORD_OP_HEARTBEAT          1
#define DISCORD_OP_IDENTIFY           2
#define DISCORD_OP_RESUME             6
#define DISCORD_OP_RECONNECT          7
#define DISCORD_OP_HELLO              10
#define DISCORD_OP_HEARTBEAT_ACK      11

/* Discord Gateway intents */
#define DISCORD_INTENT_GUILD_MESSAGES     (1 << 9)
#define DISCORD_INTENT_DIRECT_MESSAGES    (1 << 12)
#define DISCORD_INTENT_MESSAGE_CONTENT    (1 << 15)

static const char *TAG = "discord";

static char s_bot_token[256] = MIMI_SECRET_DISCORD_TOKEN;
static esp_websocket_client_handle_t s_ws = NULL;
static int64_t s_heartbeat_interval_ms = 41250;
static int64_t s_last_heartbeat_us = 0;
static int64_t s_last_heartbeat_ack_us = 0;
static char s_session_id[128] = {0};
static EventGroupHandle_t s_events = NULL;
static bool s_identified = false;

#define BIT_CONNECTED   (1 << 0)
#define BIT_HELLO       (1 << 1)

static uint64_t s_seen_msg_keys[DISCORD_DEDUP_CACHE_SIZE] = {0};
static size_t s_seen_msg_idx = 0;

/* ── FNV-1a dedup (same as telegram_bot.c) ───────────────────── */

static uint64_t fnv1a64(const char *s)
{
    uint64_t h = 1469598103934665603ULL;
    if (!s) return h;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t make_msg_key(const char *channel_id, const char *msg_id)
{
    uint64_t h = fnv1a64(channel_id);
    uint64_t m = fnv1a64(msg_id);
    return h ^ (m * 1099511628211ULL);
}

static bool seen_msg_contains(uint64_t key)
{
    for (size_t i = 0; i < DISCORD_DEDUP_CACHE_SIZE; i++) {
        if (s_seen_msg_keys[i] == key) return true;
    }
    return false;
}

static void seen_msg_insert(uint64_t key)
{
    s_seen_msg_keys[s_seen_msg_idx] = key;
    s_seen_msg_idx = (s_seen_msg_idx + 1) % DISCORD_DEDUP_CACHE_SIZE;
}

/* ── HTTP response accumulator ───────────────────────────────── */

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} http_resp_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_resp_t *resp = (http_resp_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (resp->len + evt->data_len >= resp->cap) {
            size_t new_cap = resp->cap * 2;
            if (new_cap < resp->len + evt->data_len + 1)
                new_cap = resp->len + evt->data_len + 1;
            char *tmp = realloc(resp->buf, new_cap);
            if (!tmp) return ESP_ERR_NO_MEM;
            resp->buf = tmp;
            resp->cap = new_cap;
        }
        memcpy(resp->buf + resp->len, evt->data, evt->data_len);
        resp->len += evt->data_len;
        resp->buf[resp->len] = '\0';
    }
    return ESP_OK;
}

/* ── REST: send message ──────────────────────────────────────── */

static esp_err_t discord_api_post(const char *path, const char *json_body, char **out_resp)
{
    if (out_resp) *out_resp = NULL;
    if (s_bot_token[0] == '\0') return ESP_ERR_INVALID_STATE;

    char url[512];
    snprintf(url, sizeof(url), "%s%s", DISCORD_API_BASE, path);

    http_resp_t resp = {
        .buf = calloc(1, 4096),
        .len = 0,
        .cap = 4096,
    };
    if (!resp.buf) return ESP_ERR_NO_MEM;

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) { free(resp.buf); return ESP_FAIL; }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char auth[300];
    snprintf(auth, sizeof(auth), "Bot %s", s_bot_token);
    esp_http_client_set_header(client, "Authorization", auth);
    if (json_body)
        esp_http_client_set_post_field(client, json_body, (int)strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);

    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "REST POST %s failed: %s", path, esp_err_to_name(err));
        free(resp.buf);
        return err;
    }

    if (status >= 400) {
        ESP_LOGE(TAG, "REST POST %s HTTP %d: %.200s", path, status, resp.buf ? resp.buf : "");
    }

    if (out_resp)
        *out_resp = resp.buf;
    else
        free(resp.buf);

    return (status >= 200 && status < 300) ? ESP_OK : ESP_FAIL;
}

/* ── Gateway event handlers ──────────────────────────────────── */

static void send_heartbeat(void)
{
    cJSON *hb = cJSON_CreateObject();
    cJSON_AddNumberToObject(hb, "op", DISCORD_OP_HEARTBEAT);
    cJSON_AddNullToObject(hb, "d");
    char *s = cJSON_PrintUnformatted(hb);
    cJSON_Delete(hb);
    if (s) {
        esp_websocket_client_send_text(s_ws, s, (int)strlen(s), portMAX_DELAY);
        ESP_LOGD(TAG, "Heartbeat sent");
        free(s);
    }
    s_last_heartbeat_us = esp_timer_get_time();
}

static void send_identify(void)
{
    cJSON *d = cJSON_CreateObject();
    cJSON_AddStringToObject(d, "token", s_bot_token);
    cJSON *props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "os", "esp-idf");
    cJSON_AddStringToObject(props, "browser", "mimiclaw");
    cJSON_AddStringToObject(props, "device", "mimiclaw");
    cJSON_AddItemToObject(d, "properties", props);
    cJSON_AddNumberToObject(d, "intents",
        DISCORD_INTENT_GUILD_MESSAGES |
        DISCORD_INTENT_DIRECT_MESSAGES |
        DISCORD_INTENT_MESSAGE_CONTENT);

    cJSON *id = cJSON_CreateObject();
    cJSON_AddNumberToObject(id, "op", DISCORD_OP_IDENTIFY);
    cJSON_AddItemToObject(id, "d", d);

    char *s = cJSON_PrintUnformatted(id);
    cJSON_Delete(id);
    if (s) {
        esp_websocket_client_send_text(s_ws, s, (int)strlen(s), portMAX_DELAY);
        ESP_LOGI(TAG, "Identify sent");
        free(s);
        s_identified = true;
    }
}

static void handle_message_create(const char *payload)
{
    cJSON *msg = cJSON_Parse(payload);
    if (!msg) return;

    cJSON *author = cJSON_GetObjectItem(msg, "author");
    if (!author) { cJSON_Delete(msg); return; }

    cJSON *author_id = cJSON_GetObjectItem(author, "id");
    cJSON *channel_id = cJSON_GetObjectItem(msg, "channel_id");
    cJSON *content = cJSON_GetObjectItem(msg, "content");
    cJSON *msg_id = cJSON_GetObjectItem(msg, "id");

    if (!content || !cJSON_IsString(content) || !content->valuestring[0]) {
        cJSON_Delete(msg);
        return;
    }
    if (!channel_id || !cJSON_IsString(channel_id)) {
        cJSON_Delete(msg);
        return;
    }

    const char *cid_str = channel_id->valuestring;
    const char *mid_str = (msg_id && cJSON_IsString(msg_id)) ? msg_id->valuestring : "";

    if (mid_str[0]) {
        uint64_t key = make_msg_key(cid_str, mid_str);
        if (seen_msg_contains(key)) {
            ESP_LOGD(TAG, "Drop duplicate msg %s in %s", mid_str, cid_str);
            cJSON_Delete(msg);
            return;
        }
        seen_msg_insert(key);
    }

    const char *aid_str = (author_id && cJSON_IsString(author_id)) ? author_id->valuestring : "";

    ESP_LOGI(TAG, "MSG %s from %s in %s: %.40s%s",
             mid_str, aid_str, cid_str,
             content->valuestring,
             strlen(content->valuestring) > 40 ? "..." : "");

    mimi_msg_t bus_msg = {0};
    strncpy(bus_msg.channel, MIMI_CHAN_DISCORD, sizeof(bus_msg.channel) - 1);
    strncpy(bus_msg.chat_id, cid_str, sizeof(bus_msg.chat_id) - 1);
    if (aid_str[0])
        strncpy(bus_msg.user_id, aid_str, sizeof(bus_msg.user_id) - 1);
    if (mid_str[0])
        strncpy(bus_msg.message_id, mid_str, sizeof(bus_msg.message_id) - 1);
    bus_msg.content = strdup(content->valuestring);

    if (bus_msg.content) {
        if (message_bus_push_inbound(&bus_msg) != ESP_OK) {
            ESP_LOGW(TAG, "Inbound queue full, drop discord message");
            free(bus_msg.content);
        }
    }

    cJSON_Delete(msg);
}

static void handle_gateway_event(int op, const char *t, const char *d)
{
    if (op == DISCORD_OP_HELLO) {
        cJSON *root = cJSON_Parse(d);
        if (root) {
            cJSON *iv = cJSON_GetObjectItem(root, "heartbeat_interval");
            if (iv && cJSON_IsNumber(iv))
                s_heartbeat_interval_ms = (int64_t)iv->valuedouble;
            cJSON_Delete(root);
        }
        ESP_LOGI(TAG, "Hello: heartbeat_interval=%" PRId64 "ms", s_heartbeat_interval_ms);
        if (s_events) xEventGroupSetBits(s_events, BIT_HELLO);

    } else if (op == DISCORD_OP_DISPATCH) {
        if (t && strcmp(t, "MESSAGE_CREATE") == 0) {
            handle_message_create(d);
        } else if (t && strcmp(t, "READY") == 0) {
            cJSON *root = cJSON_Parse(d);
            if (root) {
                cJSON *sid = cJSON_GetObjectItem(root, "session_id");
                if (sid && cJSON_IsString(sid))
                    strncpy(s_session_id, sid->valuestring, sizeof(s_session_id) - 1);
                cJSON_Delete(root);
            }
            ESP_LOGI(TAG, "Ready, session_id=%.20s", s_session_id);
        }

    } else if (op == DISCORD_OP_HEARTBEAT_ACK) {
        s_last_heartbeat_ack_us = esp_timer_get_time();
        ESP_LOGD(TAG, "Heartbeat ACK");

    } else if (op == DISCORD_OP_RECONNECT) {
        ESP_LOGW(TAG, "Gateway requested reconnect");
        esp_websocket_client_close(s_ws, portMAX_DELAY);

    } else if (op == 9) {
        ESP_LOGW(TAG, "Invalid session, re-identifying");
        s_identified = false;
    }
}

/* ── WebSocket callbacks ─────────────────────────────────────── */

static void ws_event_handler(void *arg, esp_event_base_t base,
                              int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WS connected to Discord Gateway");
        s_identified = false;
        if (s_events) xEventGroupSetBits(s_events, BIT_CONNECTED);
        break;

    case WEBSOCKET_EVENT_DATA:
        if (!data->data_len || data->data_ptr[0] == '\0') break;
        {
            char *json = malloc(data->data_len + 1);
            if (!json) break;
            memcpy(json, data->data_ptr, data->data_len);
            json[data->data_len] = '\0';

            cJSON *root = cJSON_Parse(json);
            if (root) {
                cJSON *op_item = cJSON_GetObjectItem(root, "op");
                cJSON *t_item = cJSON_GetObjectItem(root, "t");
                cJSON *d_item = cJSON_GetObjectItem(root, "d");

                int op = (op_item && cJSON_IsNumber(op_item)) ? (int)op_item->valuedouble : -1;
                const char *t = (t_item && cJSON_IsString(t_item)) ? t_item->valuestring : NULL;
                char *d_str = d_item ? cJSON_PrintUnformatted(d_item) : NULL;

                handle_gateway_event(op, t, d_str ? d_str : "");

                free(d_str);
                cJSON_Delete(root);
            }
            free(json);
        }
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WS disconnected");
        s_identified = false;
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WS error");
        break;

    default:
        break;
    }
}

/* ── Gateway task ────────────────────────────────────────────── */

static void discord_gw_task(void *arg)
{
    ESP_LOGI(TAG, "Gateway task started");

    while (1) {
        if (s_bot_token[0] == '\0') {
            ESP_LOGW(TAG, "No bot token, waiting...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (s_events) xEventGroupClearBits(s_events, BIT_CONNECTED | BIT_HELLO);

        esp_websocket_client_config_t ws_cfg = {
            .uri = DISCORD_GATEWAY_URL,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .buffer_size = 4096,
            .buffer_size_tx = 2048,
            .reconnect_timeout_ms = 5000,
            .network_timeout_ms = 10000,
        };

        s_ws = esp_websocket_client_init(&ws_cfg);
        if (!s_ws) {
            ESP_LOGE(TAG, "Failed to init WS client");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
        esp_err_t err = esp_websocket_client_start(s_ws);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "WS start failed: %s", esp_err_to_name(err));
            esp_websocket_client_destroy(s_ws);
            s_ws = NULL;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        EventBits_t bits = xEventGroupWaitBits(s_events,
            BIT_CONNECTED | BIT_HELLO,
            pdFALSE, pdTRUE,
            pdMS_TO_TICKS(15000));

        if (!(bits & BIT_CONNECTED)) {
            ESP_LOGW(TAG, "WS connect timeout, retrying");
            esp_websocket_client_stop(s_ws);
            esp_websocket_client_destroy(s_ws);
            s_ws = NULL;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        bits = xEventGroupWaitBits(s_events, BIT_HELLO,
            pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));

        if (!(bits & BIT_HELLO)) {
            ESP_LOGW(TAG, "Hello timeout, retrying");
            esp_websocket_client_stop(s_ws);
            esp_websocket_client_destroy(s_ws);
            s_ws = NULL;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        send_identify();

        s_last_heartbeat_us = esp_timer_get_time();
        s_last_heartbeat_ack_us = esp_timer_get_time();

        while (esp_websocket_client_is_connected(s_ws)) {
            int64_t now = esp_timer_get_time();
            int64_t elapsed_ms = (now - s_last_heartbeat_us) / 1000;

            if (elapsed_ms >= s_heartbeat_interval_ms) {
                int64_t ack_elapsed_ms = (now - s_last_heartbeat_ack_us) / 1000;
                if (ack_elapsed_ms > s_heartbeat_interval_ms * 2) {
                    ESP_LOGW(TAG, "No heartbeat ACK, reconnecting");
                    break;
                }
                send_heartbeat();
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        ESP_LOGW(TAG, "Gateway loop ended, cleaning up");
        esp_websocket_client_stop(s_ws);
        esp_websocket_client_destroy(s_ws);
        s_ws = NULL;
        s_identified = false;

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ── Public API ──────────────────────────────────────────────── */

esp_err_t discord_bot_init(void)
{
    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_DISCORD, NVS_READONLY, &nvs) == ESP_OK) {
        char tmp[256] = {0};
        size_t len = sizeof(tmp);
        if (nvs_get_str(nvs, DISCORD_NVS_KEY_TOKEN, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_bot_token, tmp, sizeof(s_bot_token) - 1);
        }
        nvs_close(nvs);
    }

    s_events = xEventGroupCreate();
    if (!s_events) return ESP_ERR_NO_MEM;

    if (s_bot_token[0])
        ESP_LOGI(TAG, "Discord bot token loaded (len=%d)", (int)strlen(s_bot_token));
    else
        ESP_LOGW(TAG, "No Discord bot token. Use CLI: set_discord_token <TOKEN>");

    return ESP_OK;
}

esp_err_t discord_bot_start(void)
{
    BaseType_t ret = xTaskCreatePinnedToCore(
        discord_gw_task, "discord_gw",
        MIMI_DISCORD_GW_STACK, NULL,
        MIMI_DISCORD_GW_PRIO, NULL,
        MIMI_AGENT_CORE);

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t discord_send_message(const char *channel_id, const char *text)
{
    if (s_bot_token[0] == '\0') {
        ESP_LOGW(TAG, "Cannot send: no bot token");
        return ESP_ERR_INVALID_STATE;
    }
    if (!channel_id || !text) return ESP_ERR_INVALID_ARG;

    size_t text_len = strlen(text);
    size_t offset = 0;
    int all_ok = 1;

    while (offset < text_len) {
        size_t chunk = text_len - offset;
        if (chunk > MIMI_DISCORD_MAX_MSG_LEN)
            chunk = MIMI_DISCORD_MAX_MSG_LEN;

        cJSON *body = cJSON_CreateObject();
        char *segment = malloc(chunk + 1);
        if (!segment) { cJSON_Delete(body); return ESP_ERR_NO_MEM; }
        memcpy(segment, text + offset, chunk);
        segment[chunk] = '\0';
        cJSON_AddStringToObject(body, "content", segment);
        free(segment);

        char *json_str = cJSON_PrintUnformatted(body);
        cJSON_Delete(body);

        if (!json_str) { all_ok = 0; offset += chunk; continue; }

        char path[256];
        snprintf(path, sizeof(path), "/channels/%s/messages", channel_id);

        ESP_LOGI(TAG, "Sending to channel %s (%d bytes)", channel_id, (int)chunk);
        char *resp = NULL;
        esp_err_t err = discord_api_post(path, json_str, &resp);
        free(json_str);
        free(resp);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Send failed to channel %s", channel_id);
            all_ok = 0;
        }

        offset += chunk;
    }

    return all_ok ? ESP_OK : ESP_FAIL;
}

esp_err_t discord_set_token(const char *token)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open(MIMI_NVS_DISCORD, NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, DISCORD_NVS_KEY_TOKEN, token));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    strncpy(s_bot_token, token, sizeof(s_bot_token) - 1);
    ESP_LOGI(TAG, "Discord bot token saved");
    return ESP_OK;
}
