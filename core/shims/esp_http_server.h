#pragma once
#include "esp_err.h"
#include <stddef.h>

typedef void *httpd_handle_t;

typedef struct httpd_req httpd_req_t;

#define HTTP_GET    0
#define HTTP_POST   1
#define HTTP_DELETE 2
#define HTTP_PUT    3

#define HTTPD_DEFAULT_CONFIG() { .server_port = 8080, .max_uri_handlers = 8 }
#define HTTPD_200_OK              200
#define HTTPD_400_BAD_REQUEST     400
#define HTTPD_404_NOT_FOUND       404
#define HTTPD_500_INTERNAL_SERVER_ERROR 500

typedef struct {
    const char *uri;
    int method;
    esp_err_t (*handler)(httpd_req_t *req);
    void *user_ctx;
} httpd_uri_t;

typedef struct {
    int server_port;
    int max_uri_handlers;
} httpd_config_t;

typedef struct httpd_req {
    httpd_handle_t handle;
    const char *uri;
    int method;
    void *user_ctx;
    int content_len;
} httpd_req_t;

#define HTTPD_WS_TYPE_TEXT   0x01
#define HTTPD_WS_TYPE_BINARY 0x02
#define HTTPD_WS_TYPE_CLOSE  0x08
#define HTTPD_WS_TYPE_PING   0x09
#define HTTPD_WS_TYPE_PONG   0x0A

typedef struct {
    uint8_t type;
    uint8_t *payload;
    size_t len;
    int final;
} httpd_ws_frame_t;

static inline esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config) {
    (void)config;
    *handle = (void*)1;
    return ESP_OK;
}

static inline esp_err_t httpd_stop(httpd_handle_t handle) {
    (void)handle;
    return ESP_OK;
}

static inline esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri) {
    (void)handle; (void)uri;
    return ESP_OK;
}

static inline esp_err_t httpd_resp_set_type(httpd_req_t *req, const char *type) {
    (void)req; (void)type;
    return ESP_OK;
}

static inline esp_err_t httpd_resp_set_hdr(httpd_req_t *req, const char *field, const char *value) {
    (void)req; (void)field; (void)value;
    return ESP_OK;
}

static inline esp_err_t httpd_resp_send(httpd_req_t *req, const char *buf, ssize_t len) {
    (void)req; (void)buf; (void)len;
    return ESP_OK;
}

static inline esp_err_t httpd_resp_send_err(httpd_req_t *req, int error, const char *msg) {
    (void)req; (void)error; (void)msg;
    return ESP_OK;
}

static inline int httpd_req_recv(httpd_req_t *req, char *buf, size_t len) {
    (void)req; (void)buf; (void)len;
    return 0;
}

static inline int httpd_req_to_sockfd(httpd_req_t *req) {
    (void)req;
    return -1;
}

static inline esp_err_t httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *frame, size_t max_len) {
    (void)req; (void)frame; (void)max_len;
    return ESP_OK;
}

static inline esp_err_t httpd_ws_send_frame_async(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame) {
    (void)hd; (void)fd; (void)frame;
    return ESP_OK;
}
