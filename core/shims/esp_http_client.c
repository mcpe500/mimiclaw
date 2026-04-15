#include "esp_http_client.h"
#include "esp_log.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "http_client";

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} http_resp_buf_t;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    http_resp_buf_t *buf = (http_resp_buf_t *)userdata;
    size_t total = size * nmemb;
    if (buf->size + total + 1 > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : 4096;
        while (new_cap < buf->size + total + 1) new_cap *= 2;
        char *tmp = realloc(buf->data, new_cap);
        if (!tmp) return 0;
        buf->data = tmp;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static void curl_init_once(void) {
    static int done = 0;
    if (!done) {
        curl_global_init(CURL_GLOBAL_ALL);
        done = 1;
    }
}

typedef struct {
    CURL *curl;
    struct curl_slist *headers;
    http_resp_buf_t resp;
    char *post_data;
    char url[1024];
    int method;
    int status_code;
} http_handle_priv_t;

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config) {
    if (!config) return NULL;
    curl_init_once();

    http_handle_priv_t *priv = calloc(1, sizeof(*priv));
    if (!priv) return NULL;

    priv->curl = curl_easy_init();
    if (!priv->curl) { free(priv); return NULL; }

    strncpy(priv->url, config->url ? config->url : "", sizeof(priv->url) - 1);
    priv->method = config->method;
    priv->headers = curl_slist_append(priv->headers, "Content-Type: application/json");

    curl_easy_setopt(priv->curl, CURLOPT_URL, priv->url);
    curl_easy_setopt(priv->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(priv->curl, CURLOPT_WRITEDATA, &priv->resp);
    curl_easy_setopt(priv->curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(priv->curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(priv->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(priv->curl, CURLOPT_SSL_VERIFYPEER, 0L);

    if (config->method == ESP_HTTP_CLIENT_METHOD_POST) {
        curl_easy_setopt(priv->curl, CURLOPT_POST, 1L);
    }

    return (esp_http_client_handle_t)priv;
}

esp_err_t esp_http_client_set_post_field(esp_http_client_handle_t client, const char *data, int len) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    if (!priv) return ESP_ERR_INVALID_ARG;

    free(priv->post_data);
    priv->post_data = NULL;

    if (len < 0) len = (int)strlen(data);
    priv->post_data = malloc((size_t)len + 1);
    if (!priv->post_data) return ESP_ERR_NO_MEM;
    memcpy(priv->post_data, data, (size_t)len);
    priv->post_data[len] = '\0';

    curl_easy_setopt(priv->curl, CURLOPT_POSTFIELDS, priv->post_data);
    curl_easy_setopt(priv->curl, CURLOPT_POSTFIELDSIZE, (long)len);
    return ESP_OK;
}

esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char *key, const char *value) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    if (!priv) return ESP_ERR_INVALID_ARG;
    char hdr[512];
    snprintf(hdr, sizeof(hdr), "%s: %s", key, value);
    priv->headers = curl_slist_append(priv->headers, hdr);
    return ESP_OK;
}

int esp_http_client_get_status_code(esp_http_client_handle_t client) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    return priv ? priv->status_code : 0;
}

int esp_http_client_get_content_length(esp_http_client_handle_t client) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    return priv ? (int)priv->resp.size : 0;
}

char *esp_http_client_get_response_body(esp_http_client_handle_t client) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    return priv ? priv->resp.data : NULL;
}

esp_err_t esp_http_client_perform(esp_http_client_handle_t client) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    if (!priv) return ESP_ERR_INVALID_ARG;

    priv->resp.size = 0;
    if (priv->resp.data) priv->resp.data[0] = '\0';

    curl_easy_setopt(priv->curl, CURLOPT_HTTPHEADER, priv->headers);
    CURLcode res = curl_easy_perform(priv->curl);

    if (res != CURLE_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", curl_easy_strerror(res));
        priv->status_code = 0;
        return ESP_FAIL;
    }

    long http_code = 0;
    curl_easy_getinfo(priv->curl, CURLINFO_RESPONSE_CODE, &http_code);
    priv->status_code = (int)http_code;

    ESP_LOGD(TAG, "HTTP %s %s -> %d (%zu bytes)",
             priv->method == ESP_HTTP_CLIENT_METHOD_POST ? "POST" : "GET",
             priv->url, priv->status_code, priv->resp.size);

    return ESP_OK;
}

void esp_http_client_cleanup(esp_http_client_handle_t client) {
    http_handle_priv_t *priv = (http_handle_priv_t *)client;
    if (!priv) return;
    if (priv->curl) curl_easy_cleanup(priv->curl);
    if (priv->headers) curl_slist_free_all(priv->headers);
    if (priv->resp.data) free(priv->resp.data);
    if (priv->post_data) free(priv->post_data);
    free(priv);
}
