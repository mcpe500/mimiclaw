#include "esp_http_client.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static const char *TAG = "esp_http_client";

typedef struct {
    CURL *curl;
    char *response;
    size_t response_len;
    size_t response_cap;
    int status_code;
    char *post_data;
} http_client_t;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    http_client_t *c = (http_client_t *)userdata;
    size_t total = size * nmemb;
    if (c->response_len + total + 1 > c->response_cap) {
        size_t new_cap = (c->response_cap == 0) ? 4096 : c->response_cap * 2;
        while (new_cap < c->response_len + total + 1) new_cap *= 2;
        char *tmp = realloc(c->response, new_cap);
        if (!tmp) return 0;
        c->response = tmp;
        c->response_cap = new_cap;
    }
    memcpy(c->response + c->response_len, ptr, total);
    c->response_len += total;
    c->response[c->response_len] = '\0';
    return total;
}

static void curl_init_once(void) {
    static int done = 0;
    if (!done) {
        curl_global_init(CURL_GLOBAL_ALL);
        done = 1;
    }
}

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config) {
    if (!config) return NULL;
    curl_init_once();

    http_client_t *c = calloc(1, sizeof(http_client_t));
    if (!c) return NULL;

    c->curl = curl_easy_init();
    if (!c->curl) { free(c); return NULL; }

    curl_easy_setopt(c->curl, CURLOPT_URL, config->url);
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, c);
    curl_easy_setopt(c->curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYPEER, 1L);

    if (config->method == ESP_HTTP_CLIENT_METHOD_POST) {
        curl_easy_setopt(c->curl, CURLOPT_POST, 1L);
    }

    ESP_LOGD(TAG, "init %s %s", config->method == ESP_HTTP_CLIENT_METHOD_POST ? "POST" : "GET", config->url);
    return (esp_http_client_handle_t)c;
}

esp_err_t esp_http_client_perform(esp_http_client_handle_t client) {
    if (!client) return ESP_ERR_INVALID_ARG;
    http_client_t *c = (http_client_t *)client;

    if (c->post_data) {
        curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, c->post_data);
    }

    c->response_len = 0;
    if (c->response) c->response[0] = '\0';

    CURLcode res = curl_easy_perform(c->curl);
    if (res != CURLE_OK) {
        ESP_LOGE(TAG, "request failed: %s", curl_easy_strerror(res));
        c->status_code = 0;
        return ESP_FAIL;
    }

    long code = 0;
    curl_easy_getinfo(c->curl, CURLINFO_RESPONSE_CODE, &code);
    c->status_code = (int)code;

    ESP_LOGD(TAG, "status %d, %zu bytes", c->status_code, c->response_len);
    return (c->status_code >= 200 && c->status_code < 300) ? ESP_OK : ESP_FAIL;
}

int esp_http_client_get_status_code(esp_http_client_handle_t client) {
    if (!client) return 0;
    return ((http_client_t *)client)->status_code;
}

void esp_http_client_cleanup(esp_http_client_handle_t client) {
    if (!client) return;
    http_client_t *c = (http_client_t *)client;
    curl_easy_cleanup(c->curl);
    free(c->response);
    free(c->post_data);
    free(c);
}
