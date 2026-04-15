#include "admin_server.h"
#include "mimi_config.h"
#include "memory/session_mgr.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "cJSON.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "admin";
static httpd_handle_t s_server = NULL;

#define ADMIN_PORT 8080

static const char *ADMIN_HTML =
"<!DOCTYPE html><html><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>MimiClaw Admin</title>"
"<style>"
"body{font-family:sans-serif;max-width:800px;margin:0 auto;padding:20px;background:#1a1a2e;color:#eee}"
"h1{color:#e94560}a{color:#0f3460}.card{background:#16213e;border-radius:8px;padding:16px;margin:8px 0}"
"pre{background:#0f3460;padding:12px;border-radius:4px;overflow-x:auto}"
"</style></head><body>"
"<h1>MimiClaw Admin Dashboard</h1>"
"<div class='card'><h3>Status</h3><pre id='status'>Loading...</pre></div>"
"<div class='card'><h3>Configuration</h3><pre id='config'>Loading...</pre></div>"
"<div class='card'><h3>Sessions</h3><pre id='sessions'>Loading...</pre></div>"
"<script>"
"async function loadJSON(id,url){const r=await fetch(url);const t=await r.text();"
"document.getElementById(id).textContent=JSON.stringify(JSON.parse(t),null,2);}"
"loadJSON('status','/api/status');loadJSON('config','/api/config');"
"loadJSON('sessions','/api/sessions');"
"</script></body></html>";

static void mask_token(const char *src, char *dst, size_t dst_size)
{
    size_t len = strlen(src);
    if (len <= 4) {
        snprintf(dst, dst_size, "****");
        return;
    }
    snprintf(dst, dst_size, "%.*s****%.*s", 2, src, 2, src + len - 2);
}

static void json_add_masked(cJSON *root, const char *key,
                            const char *ns, const char *nvs_key,
                            const char *build_val)
{
    char value[256] = {0};
    bool found = false;

    nvs_handle_t nvs;
    if (nvs_open(ns, NVS_READONLY, &nvs) == ESP_OK) {
        size_t len = sizeof(value);
        if (nvs_get_str(nvs, nvs_key, value, &len) == ESP_OK) {
            found = true;
        }
        nvs_close(nvs);
    }

    if (!found && build_val) {
        strlcpy(value, build_val, sizeof(value));
    }

    if (strlen(value) > 0) {
        char masked[256];
        mask_token(value, masked, sizeof(masked));
        cJSON_AddStringToObject(root, key, masked);
    } else {
        cJSON_AddStringToObject(root, key, "");
    }
}

static esp_err_t http_get_root(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, ADMIN_HTML, strlen(ADMIN_HTML));
}

static esp_err_t http_get_status(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    int64_t uptime_s = (int64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;
    cJSON_AddNumberToObject(root, "uptime_s", (double)uptime_s);
    cJSON_AddNumberToObject(root, "free_heap", (double)esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "min_heap", (double)esp_get_minimum_free_heap_size());

    cJSON *channels = cJSON_CreateObject();
    cJSON_AddBoolToObject(channels, "telegram", true);
    cJSON_AddBoolToObject(channels, "feishu", true);
    cJSON_AddBoolToObject(channels, "discord", true);
    cJSON_AddBoolToObject(channels, "websocket", true);
    cJSON_AddBoolToObject(channels, "whatsapp", true);
    cJSON_AddBoolToObject(channels, "slack", true);
    cJSON_AddItemToObject(root, "channels", channels);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, json, strlen(json));
    free(json);
    return ret;
}

static esp_err_t http_get_config(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    json_add_masked(root, "ssid", MIMI_NVS_WIFI, MIMI_NVS_KEY_SSID, MIMI_SECRET_WIFI_SSID);
    json_add_masked(root, "password", MIMI_NVS_WIFI, MIMI_NVS_KEY_PASS, MIMI_SECRET_WIFI_PASS);
    json_add_masked(root, "api_key", MIMI_NVS_LLM, MIMI_NVS_KEY_API_KEY, MIMI_SECRET_API_KEY);
    cJSON_AddStringToObject(root, "model", MIMI_SECRET_MODEL);
    cJSON_AddStringToObject(root, "provider", MIMI_SECRET_MODEL_PROVIDER);
    json_add_masked(root, "tg_token", MIMI_NVS_TG, MIMI_NVS_KEY_TG_TOKEN, MIMI_SECRET_TG_TOKEN);
    json_add_masked(root, "proxy_host", MIMI_NVS_PROXY, MIMI_NVS_KEY_PROXY_HOST, MIMI_SECRET_PROXY_HOST);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    esp_err_t ret = httpd_resp_send(req, json, strlen(json));
    free(json);
    return ret;
}

static void nvs_set_str_safe(const char *ns, const char *key, const char *value)
{
    nvs_handle_t nvs;
    if (nvs_open(ns, NVS_READWRITE, &nvs) == ESP_OK) {
        if (value && value[0] != '\0') {
            nvs_set_str(nvs, key, value);
        } else {
            nvs_erase_key(nvs, key);
        }
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

static esp_err_t http_post_config(httpd_req_t *req)
{
    int total_len = req->content_len;
    if (total_len <= 0 || total_len > 4096) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad length");
        return ESP_FAIL;
    }

    char *buf = calloc(1, total_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }

    int received = 0;
    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, total_len - received);
        if (ret <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Recv error");
            return ESP_FAIL;
        }
        received += ret;
    }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *item;
    item = cJSON_GetObjectItem(root, "ssid");
    if (item && cJSON_IsString(item)) nvs_set_str_safe(MIMI_NVS_WIFI, MIMI_NVS_KEY_SSID, item->valuestring);
    item = cJSON_GetObjectItem(root, "password");
    if (item && cJSON_IsString(item)) nvs_set_str_safe(MIMI_NVS_WIFI, MIMI_NVS_KEY_PASS, item->valuestring);
    item = cJSON_GetObjectItem(root, "api_key");
    if (item && cJSON_IsString(item)) nvs_set_str_safe(MIMI_NVS_LLM, MIMI_NVS_KEY_API_KEY, item->valuestring);
    item = cJSON_GetObjectItem(root, "model");
    if (item && cJSON_IsString(item)) nvs_set_str_safe(MIMI_NVS_LLM, MIMI_NVS_KEY_MODEL, item->valuestring);
    item = cJSON_GetObjectItem(root, "provider");
    if (item && cJSON_IsString(item)) nvs_set_str_safe(MIMI_NVS_LLM, MIMI_NVS_KEY_PROVIDER, item->valuestring);

    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

static esp_err_t http_get_sessions(httpd_req_t *req)
{
    cJSON *arr = cJSON_CreateArray();

    cJSON *s1 = cJSON_CreateObject();
    cJSON_AddStringToObject(s1, "id", "tg_12345");
    cJSON_AddStringToObject(s1, "channel", "telegram");
    cJSON_AddNumberToObject(s1, "msgs", 3);
    cJSON_AddItemToArray(arr, s1);

    cJSON *s2 = cJSON_CreateObject();
    cJSON_AddStringToObject(s2, "id", "ws_client1");
    cJSON_AddStringToObject(s2, "channel", "websocket");
    cJSON_AddNumberToObject(s2, "msgs", 1);
    cJSON_AddItemToArray(arr, s2);

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, json, strlen(json));
    free(json);
    return ret;
}

static esp_err_t http_delete_session(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/sessions/";
    if (strncmp(uri, prefix, strlen(prefix)) != 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad path");
        return ESP_FAIL;
    }

    const char *session_id = uri + strlen(prefix);
    if (strlen(session_id) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing session id");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Clearing session: %s", session_id);
    session_clear(session_id, "");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

esp_err_t admin_server_start(void)
{
    if (s_server) {
        ESP_LOGW(TAG, "Admin server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = ADMIN_PORT;
    config.ctrl_port = ADMIN_PORT + 1;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start admin server: %s", esp_err_to_name(ret));
        return ret;
    }

    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = http_get_root };
    httpd_register_uri_handler(s_server, &uri_root);

    httpd_uri_t uri_status = { .uri = "/api/status", .method = HTTP_GET, .handler = http_get_status };
    httpd_register_uri_handler(s_server, &uri_status);

    httpd_uri_t uri_config_get = { .uri = "/api/config", .method = HTTP_GET, .handler = http_get_config };
    httpd_register_uri_handler(s_server, &uri_config_get);

    httpd_uri_t uri_config_post = { .uri = "/api/config", .method = HTTP_POST, .handler = http_post_config };
    httpd_register_uri_handler(s_server, &uri_config_post);

    httpd_uri_t uri_sessions = { .uri = "/api/sessions", .method = HTTP_GET, .handler = http_get_sessions };
    httpd_register_uri_handler(s_server, &uri_sessions);

    httpd_uri_t uri_session_del = { .uri = "/api/sessions/*", .method = HTTP_DELETE, .handler = http_delete_session };
    httpd_register_uri_handler(s_server, &uri_session_del);

    ESP_LOGI(TAG, "Admin server started on port %d", ADMIN_PORT);
    return ESP_OK;
}

esp_err_t admin_server_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "Admin server stopped");
    }
    return ESP_OK;
}
