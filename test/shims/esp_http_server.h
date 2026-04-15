#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

typedef void *httpd_handle_t;

typedef struct {
    int server_port;
    int ctrl_port;
    int max_uri_handlers;
    bool lru_purge_enable;
} httpd_config_t;

typedef struct {
    const char *uri;
    int method;
    void (*handler)(void *req);
} httpd_uri_t;

typedef struct {
    const char *uri;
    int content_len;
    void *user_ctx;
} httpd_req_t;

#define HTTPD_400_BAD_REQUEST 400
#define HTTPD_500_INTERNAL_SERVER_ERROR 500
#define HTTP_GET 0
#define HTTP_POST 1

static inline httpd_config_t HTTPD_DEFAULT_CONFIG(void) {
    httpd_config_t cfg = {0};
    return cfg;
}

static inline httpd_handle_t httpd_start(const httpd_config_t *config) { (void)config; return (httpd_handle_t)1; }
static inline esp_err_t httpd_stop(httpd_handle_t server) { (void)server; return ESP_OK; }
static inline esp_err_t httpd_register_uri_handler(httpd_handle_t server, const httpd_uri_t *uri_handler) { (void)server; (void)uri_handler; return ESP_OK; }
static inline esp_err_t httpd_resp_set_type(httpd_req_t *req, const char *type) { (void)req; (void)type; return ESP_OK; }
static inline esp_err_t httpd_resp_send(httpd_req_t *req, const char *buf, size_t len) { (void)req; (void)buf; (void)len; return ESP_OK; }
static inline esp_err_t httpd_resp_set_hdr(httpd_req_t *req, const char *name, const char *value) { (void)req; (void)name; (void)value; return ESP_OK; }
static inline esp_err_t httpd_resp_send_err(httpd_req_t *req, int err_code, const char *msg) { (void)req; (void)err_code; (void)msg; return ESP_OK; }
static inline int httpd_req_recv(httpd_req_t *req, void *buf, size_t len) { (void)req; (void)buf; (void)len; return 0; }
