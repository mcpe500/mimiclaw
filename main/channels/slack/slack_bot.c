#include "slack_bot.h"
#include "mimi_config.h"
#include "bus/message_bus.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "nvs.h"
#include "cJSON.h"

static const char *TAG = "slack";

#define SLACK_SOCKET_URL    "wss://wss-primary.slack.com/socket"
#define SLACK_POST_URL      "https://slack.com/api/chat.postMessage"
#define SLACK_DEDUP_SIZE    64

#ifndef MIMI_CHAN_SLACK
#define MIMI_CHAN_SLACK     "slack"
#endif

#ifndef MIMI_NVS_SLACK
#define MIMI_NVS_SLACK      "slack_config"
#endif

#ifndef MIMI_NVS_KEY_SLACK_APP_TOKEN
#define MIMI_NVS_KEY_SLACK_APP_TOKEN "app_token"
#endif

#ifndef MIMI_NVS_KEY_SLACK_BOT_TOKEN
#define MIMI_NVS_KEY_SLACK_BOT_TOKEN "bot_token"
#endif

static char s_app_token[128] = MIMI_SECRET_SLACK_APP_TOKEN;
static char s_bot_token[128] = MIMI_SECRET_SLACK_BOT_TOKEN;
static esp_websocket_client_handle_t s_ws_client = NULL;
static TaskHandle_t s_ws_task = NULL;
static bool s_ws_connected = false;

static uint64_t s_seen_keys[SLACK_DEDUP_SIZE] = {0};
static size_t s_seen_idx = 0;

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

static bool dedup_check(const char *id)
{
    uint64_t key = fnv1a64(id);
    for (size_t i = 0; i < SLACK_DEDUP_SIZE; i++) {
        if (s_seen_keys[i] == key) return true;
    }
    s_seen_keys[s_seen_idx] = key;
    s_seen_idx = (s_seen_idx + 1) % SLACK_DEDUP_SIZE;
    return false;
}

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
            if (new_cap < resp->len + evt->data_len + 1) {
                new_cap = resp->len + evt->data_len + 1;
            }
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

static void handle_slack_message(const char *user, const char *channel, const char *text)
{
    if (!text || text[0] == '\0') return;

    ESP_LOGI(TAG, "Message from %s in %s: %.60s%s",
             user ? user : "", channel ? channel : "",
             text, strlen(text) > 60 ? "..." : "");

    mimi_msg_t msg = {0};
    strncpy(msg.channel, MIMI_CHAN_SLACK, sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, channel ? channel : "", sizeof(msg.chat_id) - 1);
    if (user) strncpy(msg.user_id, user, sizeof(msg.user_id) - 1);
    msg.content = strdup(text);

    if (msg.content) {
        if (message_bus_push_inbound(&msg) != ESP_OK) {
            ESP_LOGW(TAG, "Inbound queue full, dropping slack message");
            free(msg.content);
        }
    }
}

static void send_ack(esp_websocket_client_handle_t client, const char *envelope_id)
{
    cJSON *ack = cJSON_CreateObject();
    cJSON_AddStringToObject(ack, "type", "ack");
    cJSON_AddStringToObject(ack, "envelope_id", envelope_id);
    char *json = cJSON_PrintUnformatted(ack);
    cJSON_Delete(ack);
    if (json) {
        esp_websocket_client_send_text(client, json, strlen(json), 1000);
        free(json);
    }
}

static void process_ws_text(const char *data, size_t len)
{
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) return;

    cJSON *type_item = cJSON_GetObjectItem(root, "type");
    if (!type_item || !cJSON_IsString(type_item)) {
        cJSON_Delete(root);
        return;
    }

    const char *type = type_item->valuestring;

    if (strcmp(type, "hello") == 0) {
        ESP_LOGI(TAG, "Socket Mode connected (hello received)");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(type, "disconnect") == 0) {
        ESP_LOGW(TAG, "Slack requested disconnect, will reconnect");
        s_ws_connected = false;
        cJSON_Delete(root);
        return;
    }

    if (strcmp(type, "events_api") == 0) {
        cJSON *envelope_id = cJSON_GetObjectItem(root, "envelope_id");
        if (envelope_id && cJSON_IsString(envelope_id)) {
            send_ack(s_ws_client, envelope_id->valuestring);
        }

        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (!payload) {
            cJSON_Delete(root);
            return;
        }

        cJSON *event = cJSON_GetObjectItem(payload, "event");
        if (!event) {
            cJSON_Delete(root);
            return;
        }

        cJSON *event_type = cJSON_GetObjectItem(event, "type");
        if (!event_type || !cJSON_IsString(event_type) ||
            strcmp(event_type->valuestring, "message") != 0) {
            cJSON_Delete(root);
            return;
        }

        cJSON *subtype = cJSON_GetObjectItem(event, "subtype");
        if (subtype && cJSON_IsString(subtype)) {
            cJSON_Delete(root);
            return;
        }

        cJSON *user_j = cJSON_GetObjectItem(event, "user");
        cJSON *channel_j = cJSON_GetObjectItem(event, "channel");
        cJSON *text_j = cJSON_GetObjectItem(event, "text");
        cJSON *ts_j = cJSON_GetObjectItem(event, "ts");

        if (!user_j || !channel_j || !text_j) {
            cJSON_Delete(root);
            return;
        }

        const char *user = cJSON_IsString(user_j) ? user_j->valuestring : "";
        const char *channel = cJSON_IsString(channel_j) ? channel_j->valuestring : "";
        const char *text = cJSON_IsString(text_j) ? text_j->valuestring : "";

        if (ts_j && cJSON_IsString(ts_j)) {
            char dedup_key[128];
            snprintf(dedup_key, sizeof(dedup_key), "%s:%s", channel, ts_j->valuestring);
            if (dedup_check(dedup_key)) {
                ESP_LOGD(TAG, "Duplicate message %s, skipping", dedup_key);
                cJSON_Delete(root);
                return;
            }
        }

        handle_slack_message(user, channel, text);
    }

    cJSON_Delete(root);
}

static void slack_ws_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)base;
    esp_websocket_event_data_t *e = (esp_websocket_event_data_t *)event_data;

    if (event_id == WEBSOCKET_EVENT_CONNECTED) {
        s_ws_connected = true;
        ESP_LOGI(TAG, "Slack WS connected");

        cJSON *hello = cJSON_CreateObject();
        cJSON_AddStringToObject(hello, "type", "hello");
        char *json = cJSON_PrintUnformatted(hello);
        cJSON_Delete(hello);
        if (json) {
            esp_websocket_client_send_text(e->client, json, strlen(json), 1000);
            free(json);
        }
    } else if (event_id == WEBSOCKET_EVENT_DISCONNECTED) {
        s_ws_connected = false;
        ESP_LOGW(TAG, "Slack WS disconnected");
    } else if (event_id == WEBSOCKET_EVENT_DATA) {
        if (e->op_code != WS_TRANSPORT_OPCODES_TEXT) return;
        if (e->data_ptr && e->data_len > 0) {
            char *copy = malloc(e->data_len + 1);
            if (copy) {
                memcpy(copy, e->data_ptr, e->data_len);
                copy[e->data_len] = '\0';
                process_ws_text(copy, e->data_len);
                free(copy);
            }
        }
    }
}

static void slack_ws_task(void *arg)
{
    (void)arg;

    while (1) {
        if (s_app_token[0] == '\0') {
            ESP_LOGW(TAG, "No Slack app token configured");
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        char ws_url[256];
        snprintf(ws_url, sizeof(ws_url),
                 "%s?appToken=%s", SLACK_SOCKET_URL, s_app_token);

        esp_websocket_client_config_t ws_cfg = {
            .uri = ws_url,
            .buffer_size = 4096,
            .task_stack = MIMI_AGENT_STACK,
            .reconnect_timeout_ms = 5000,
            .network_timeout_ms = 10000,
            .disable_auto_reconnect = false,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        s_ws_client = esp_websocket_client_init(&ws_cfg);
        if (!s_ws_client) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        esp_websocket_register_events(s_ws_client, WEBSOCKET_EVENT_ANY,
                                       slack_ws_event_handler, NULL);
        esp_err_t err = esp_websocket_client_start(s_ws_client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "WS start failed: %s", esp_err_to_name(err));
            esp_websocket_client_destroy(s_ws_client);
            s_ws_client = NULL;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        while (esp_websocket_client_is_connected(s_ws_client) || s_ws_connected) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        esp_websocket_client_stop(s_ws_client);
        esp_websocket_client_destroy(s_ws_client);
        s_ws_client = NULL;
        s_ws_connected = false;
        ESP_LOGI(TAG, "Reconnecting in 3s...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

esp_err_t slack_bot_init(void)
{
    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_SLACK, NVS_READONLY, &nvs) == ESP_OK) {
        char tmp[128] = {0};
        size_t len = sizeof(tmp);

        if (nvs_get_str(nvs, MIMI_NVS_KEY_SLACK_APP_TOKEN, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_app_token, tmp, sizeof(s_app_token) - 1);
        }
        len = sizeof(tmp);
        if (nvs_get_str(nvs, MIMI_NVS_KEY_SLACK_BOT_TOKEN, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_bot_token, tmp, sizeof(s_bot_token) - 1);
        }
        nvs_close(nvs);
    }

    if (s_app_token[0] && s_bot_token[0]) {
        ESP_LOGI(TAG, "Slack credentials loaded (app_token=%.8s...)", s_app_token);
    } else {
        ESP_LOGW(TAG, "No Slack credentials. Use slack_set_token to configure.");
    }

    return ESP_OK;
}

esp_err_t slack_bot_start(void)
{
    if (s_app_token[0] == '\0') {
        ESP_LOGW(TAG, "Slack not configured, skipping WebSocket start");
        return ESP_OK;
    }
    if (s_ws_task) {
        ESP_LOGW(TAG, "Slack WS task already running");
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
        slack_ws_task,
        "slack_ws",
        MIMI_AGENT_STACK,
        NULL,
        5,
        &s_ws_task,
        MIMI_AGENT_CORE);

    if (ok != pdPASS) {
        s_ws_task = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Slack Socket Mode enabled");
    return ESP_OK;
}

esp_err_t slack_send_message(const char *channel_id, const char *text)
{
    if (!channel_id || !text) return ESP_ERR_INVALID_ARG;
    if (s_bot_token[0] == '\0') {
        ESP_LOGW(TAG, "Cannot send: no bot token configured");
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "channel", channel_id);
    cJSON_AddStringToObject(body, "text", text);
    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!json_str) return ESP_ERR_NO_MEM;

    http_resp_t resp = { .buf = calloc(1, 4096), .len = 0, .cap = 4096 };
    if (!resp.buf) { free(json_str); return ESP_ERR_NO_MEM; }

    esp_http_client_config_t config = {
        .url = SLACK_POST_URL,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) { free(json_str); free(resp.buf); return ESP_FAIL; }

    char auth[192];
    snprintf(auth, sizeof(auth), "Bearer %s", s_bot_token);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", "application/json; charset=utf-8");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    free(json_str);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Send HTTP failed: %s", esp_err_to_name(err));
        free(resp.buf);
        return err;
    }

    cJSON *root = cJSON_Parse(resp.buf);
    free(resp.buf);

    esp_err_t ret = ESP_FAIL;
    if (root) {
        cJSON *ok_item = cJSON_GetObjectItem(root, "ok");
        if (ok_item && cJSON_IsBool(ok_item) && cJSON_IsTrue(ok_item)) {
            ESP_LOGI(TAG, "Sent to %s", channel_id);
            ret = ESP_OK;
        } else {
            cJSON *error = cJSON_GetObjectItem(root, "error");
            ESP_LOGW(TAG, "Send failed: %s",
                     error ? error->valuestring : "unknown");
        }
        cJSON_Delete(root);
    }

    return ret;
}

esp_err_t slack_set_token(const char *token)
{
    if (!token) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(MIMI_NVS_SLACK, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    if (strncmp(token, "xapp-", 5) == 0) {
        ESP_ERROR_CHECK(nvs_set_str(nvs, MIMI_NVS_KEY_SLACK_APP_TOKEN, token));
        strncpy(s_app_token, token, sizeof(s_app_token) - 1);
    } else if (strncmp(token, "xoxb-", 5) == 0) {
        ESP_ERROR_CHECK(nvs_set_str(nvs, MIMI_NVS_KEY_SLACK_BOT_TOKEN, token));
        strncpy(s_bot_token, token, sizeof(s_bot_token) - 1);
    } else {
        nvs_close(nvs);
        ESP_LOGW(TAG, "Unrecognized token prefix");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
    ESP_LOGI(TAG, "Slack token saved (%.5s...)", token);
    return ESP_OK;
}
