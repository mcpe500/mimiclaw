#include "whatsapp_bot.h"
#include "mimi_config.h"
#include "bus/message_bus.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

static const char *TAG = "whatsapp";

#ifndef MIMI_CHAN_WHATSAPP
#define MIMI_CHAN_WHATSAPP       "whatsapp"
#endif

#ifndef MIMI_NVS_WHATSAPP
#define MIMI_NVS_WHATSAPP        "whatsapp_config"
#endif

#ifndef MIMI_SECRET_WHATSAPP_TOKEN
#define MIMI_SECRET_WHATSAPP_TOKEN        ""
#endif

#ifndef MIMI_SECRET_WHATSAPP_PHONE_ID
#define MIMI_SECRET_WHATSAPP_PHONE_ID    ""
#endif

#ifndef MIMI_WHATSAPP_POLL_INTERVAL_MS
#define MIMI_WHATSAPP_POLL_INTERVAL_MS   5000
#endif

#ifndef MIMI_WHATSAPP_POLL_STACK
#define MIMI_WHATSAPP_POLL_STACK         (12 * 1024)
#endif

#ifndef MIMI_WHATSAPP_POLL_PRIO
#define MIMI_WHATSAPP_POLL_PRIO          5
#endif

#ifndef MIMI_WHATSAPP_POLL_CORE
#define MIMI_WHATSAPP_POLL_CORE          0
#endif

#ifndef MIMI_WHATSAPP_API_VERSION
#define MIMI_WHATSAPP_API_VERSION        "v18.0"
#endif

#define WA_BASE_URL          "https://graph.facebook.com/" MIMI_WHATSAPP_API_VERSION
#define WA_DEDUP_CACHE_SIZE  64
#define WA_NVS_KEY_TOKEN     "access_token"
#define WA_NVS_KEY_PHONE_ID  "phone_number_id"
#define WA_NVS_KEY_VERIFY    "verify_token"

static char s_access_token[256] = MIMI_SECRET_WHATSAPP_TOKEN;
static char s_phone_number_id[64] = MIMI_SECRET_WHATSAPP_PHONE_ID;
static char s_verify_token[128] = {0};

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} http_resp_t;

static char s_seen_ids[WA_DEDUP_CACHE_SIZE][64];
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

static bool dedup_contains(const char *msg_id)
{
    for (size_t i = 0; i < WA_DEDUP_CACHE_SIZE; i++) {
        if (s_seen_ids[i][0] != '\0' && strcmp(s_seen_ids[i], msg_id) == 0) {
            return true;
        }
    }
    return false;
}

static void dedup_insert(const char *msg_id)
{
    strncpy(s_seen_ids[s_seen_idx], msg_id, sizeof(s_seen_ids[s_seen_idx]) - 1);
    s_seen_ids[s_seen_idx][sizeof(s_seen_ids[s_seen_idx]) - 1] = '\0';
    s_seen_idx = (s_seen_idx + 1) % WA_DEDUP_CACHE_SIZE;
}

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

static char *wa_api_get(const char *path)
{
    char url[512];
    snprintf(url, sizeof(url), "%s/%s/%s", WA_BASE_URL, s_phone_number_id, path);

    http_resp_t resp = { .buf = calloc(1, 4096), .len = 0, .cap = 4096 };
    if (!resp.buf) return NULL;

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
    if (!client) { free(resp.buf); return NULL; }

    char auth[300];
    snprintf(auth, sizeof(auth), "Bearer %s", s_access_token);
    esp_http_client_set_header(client, "Authorization", auth);

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GET %s failed: %s", path, esp_err_to_name(err));
        free(resp.buf);
        return NULL;
    }

    return resp.buf;
}

static char *wa_api_post(const char *path, const char *json_body)
{
    char url[512];
    snprintf(url, sizeof(url), "%s/%s/%s", WA_BASE_URL, s_phone_number_id, path);

    http_resp_t resp = { .buf = calloc(1, 4096), .len = 0, .cap = 4096 };
    if (!resp.buf) return NULL;

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
    if (!client) { free(resp.buf); return NULL; }

    char auth[300];
    snprintf(auth, sizeof(auth), "Bearer %s", s_access_token);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, json_body, (int)strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "POST %s failed: %s", path, esp_err_to_name(err));
        free(resp.buf);
        return NULL;
    }

    return resp.buf;
}

static void process_incoming_messages(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return;

    cJSON *entry_arr = cJSON_GetObjectItem(root, "entry");
    if (!cJSON_IsArray(entry_arr)) { cJSON_Delete(root); return; }

    cJSON *entry;
    cJSON_ArrayForEach(entry, entry_arr) {
        cJSON *changes = cJSON_GetObjectItem(entry, "changes");
        if (!cJSON_IsArray(changes)) continue;

        cJSON *change;
        cJSON_ArrayForEach(change, changes) {
            cJSON *value = cJSON_GetObjectItem(change, "value");
            if (!value) continue;

            cJSON *messages = cJSON_GetObjectItem(value, "messages");
            if (!cJSON_IsArray(messages)) continue;

            cJSON *msg;
            cJSON_ArrayForEach(msg, messages) {
                cJSON *msg_id_obj = cJSON_GetObjectItem(msg, "id");
                if (!msg_id_obj || !cJSON_IsString(msg_id_obj)) continue;

                const char *msg_id = msg_id_obj->valuestring;
                if (dedup_contains(msg_id)) {
                    ESP_LOGD(TAG, "Dedup skip: %s", msg_id);
                    continue;
                }
                dedup_insert(msg_id);

                cJSON *from_obj = cJSON_GetObjectItem(msg, "from");
                cJSON *type_obj = cJSON_GetObjectItem(msg, "type");
                if (!from_obj || !cJSON_IsString(from_obj)) continue;
                if (!type_obj || !cJSON_IsString(type_obj)) continue;

                const char *from = from_obj->valuestring;
                const char *type = type_obj->valuestring;

                const char *text = NULL;
                if (strcmp(type, "text") == 0) {
                    cJSON *text_obj = cJSON_GetObjectItem(msg, "text");
                    if (text_obj) {
                        cJSON *body = cJSON_GetObjectItem(text_obj, "body");
                        if (body && cJSON_IsString(body)) {
                            text = body->valuestring;
                        }
                    }
                }

                if (!text) continue;

                ESP_LOGI(TAG, "Message from %s: %.60s%s", from, text,
                         strlen(text) > 60 ? "..." : "");

                mimi_msg_t bus_msg = {0};
                strncpy(bus_msg.channel, MIMI_CHAN_WHATSAPP, sizeof(bus_msg.channel) - 1);
                strncpy(bus_msg.chat_id, from, sizeof(bus_msg.chat_id) - 1);
                strncpy(bus_msg.message_id, msg_id, sizeof(bus_msg.message_id) - 1);
                bus_msg.content = strdup(text);
                if (bus_msg.content) {
                    if (message_bus_push_inbound(&bus_msg) != ESP_OK) {
                        ESP_LOGW(TAG, "Inbound queue full, dropping message");
                        free(bus_msg.content);
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
}

static void whatsapp_poll_task(void *arg)
{
    ESP_LOGI(TAG, "WhatsApp polling task started");

    while (1) {
        if (s_access_token[0] == '\0' || s_phone_number_id[0] == '\0') {
            ESP_LOGW(TAG, "No credentials configured, waiting...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        char *resp = wa_api_get("messages");
        if (resp) {
            process_incoming_messages(resp);
            free(resp);
        } else {
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(MIMI_WHATSAPP_POLL_INTERVAL_MS));
    }
}

esp_err_t whatsapp_bot_init(void)
{
    memset(s_seen_ids, 0, sizeof(s_seen_ids));
    s_seen_idx = 0;

    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_WHATSAPP, NVS_READONLY, &nvs) == ESP_OK) {
        char tmp[256] = {0};
        size_t len = sizeof(tmp);
        if (nvs_get_str(nvs, WA_NVS_KEY_TOKEN, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_access_token, tmp, sizeof(s_access_token) - 1);
        }

        len = sizeof(tmp);
        if (nvs_get_str(nvs, WA_NVS_KEY_PHONE_ID, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_phone_number_id, tmp, sizeof(s_phone_number_id) - 1);
        }

        len = sizeof(tmp);
        if (nvs_get_str(nvs, WA_NVS_KEY_VERIFY, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_verify_token, tmp, sizeof(s_verify_token) - 1);
        }

        nvs_close(nvs);
    }

    if (s_access_token[0] && s_phone_number_id[0]) {
        ESP_LOGI(TAG, "WhatsApp credentials loaded (phone_id=%s)", s_phone_number_id);
    } else {
        ESP_LOGW(TAG, "No WhatsApp credentials. Use CLI: set_wa_creds <TOKEN> <PHONE_ID>");
    }

    return ESP_OK;
}

esp_err_t whatsapp_bot_start(void)
{
    if (s_access_token[0] == '\0' || s_phone_number_id[0] == '\0') {
        ESP_LOGW(TAG, "WhatsApp not configured, skipping poll start");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        whatsapp_poll_task, "wa_poll",
        MIMI_WHATSAPP_POLL_STACK, NULL,
        MIMI_WHATSAPP_POLL_PRIO, NULL,
        MIMI_WHATSAPP_POLL_CORE);

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t whatsapp_send_message(const char *phone_number, const char *text)
{
    if (s_access_token[0] == '\0' || s_phone_number_id[0] == '\0') {
        ESP_LOGW(TAG, "Cannot send: no credentials configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (!phone_number || !text) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "messaging_product", "whatsapp");
    cJSON_AddStringToObject(body, "to", phone_number);
    cJSON_AddStringToObject(body, "type", "text");

    cJSON *text_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(text_obj, "body", text);
    cJSON_AddItemToObject(body, "text", text_obj);

    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (!json_str) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "Sending to %s (%d bytes)", phone_number, (int)strlen(text));
    char *resp = wa_api_post("messages", json_str);
    free(json_str);

    esp_err_t result = ESP_FAIL;
    if (resp) {
        cJSON *root = cJSON_Parse(resp);
        if (root) {
            cJSON *msg_id = cJSON_GetObjectItem(root, "message_id");
            cJSON *messages = cJSON_GetObjectItem(root, "messages");
            if ((msg_id && cJSON_IsString(msg_id)) ||
                (messages && cJSON_IsArray(messages))) {
                result = ESP_OK;
                ESP_LOGI(TAG, "Send success to %s", phone_number);
            } else {
                cJSON *error = cJSON_GetObjectItem(root, "error");
                if (error) {
                    cJSON *msg = cJSON_GetObjectItem(error, "message");
                    ESP_LOGE(TAG, "Send error: %s", msg && cJSON_IsString(msg) ? msg->valuestring : "unknown");
                }
            }
            cJSON_Delete(root);
        }
        free(resp);
    }

    return result;
}

esp_err_t whatsapp_set_credentials(const char *access_token, const char *phone_number_id)
{
    if (!access_token || !phone_number_id) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_WHATSAPP, NVS_READWRITE, &nvs) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for WhatsApp credentials");
        return ESP_FAIL;
    }

    if (nvs_set_str(nvs, WA_NVS_KEY_TOKEN, access_token) != ESP_OK ||
        nvs_set_str(nvs, WA_NVS_KEY_PHONE_ID, phone_number_id) != ESP_OK ||
        nvs_commit(nvs) != ESP_OK) {
        nvs_close(nvs);
        return ESP_FAIL;
    }
    nvs_close(nvs);

    strncpy(s_access_token, access_token, sizeof(s_access_token) - 1);
    s_access_token[sizeof(s_access_token) - 1] = '\0';
    strncpy(s_phone_number_id, phone_number_id, sizeof(s_phone_number_id) - 1);
    s_phone_number_id[sizeof(s_phone_number_id) - 1] = '\0';

    ESP_LOGI(TAG, "WhatsApp credentials saved");
    return ESP_OK;
}

esp_err_t whatsapp_set_verify_token(const char *verify_token)
{
    if (!verify_token) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_WHATSAPP, NVS_READWRITE, &nvs) != ESP_OK) {
        return ESP_FAIL;
    }

    if (nvs_set_str(nvs, WA_NVS_KEY_VERIFY, verify_token) != ESP_OK ||
        nvs_commit(nvs) != ESP_OK) {
        nvs_close(nvs);
        return ESP_FAIL;
    }
    nvs_close(nvs);

    strncpy(s_verify_token, verify_token, sizeof(s_verify_token) - 1);
    s_verify_token[sizeof(s_verify_token) - 1] = '\0';

    ESP_LOGI(TAG, "WhatsApp verify token saved");
    return ESP_OK;
}
