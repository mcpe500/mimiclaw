#pragma once
#include "esp_err.h"

typedef struct esp_tls esp_tls_t;

#define ESP_TLS_CONNECTING 0
#define ESP_TLS_ERR_SSL_WANT_WRITE -1
#define ESP_TLS_ERR_SSL_WANT_READ  -2

typedef struct {
    const char *cacert_pem;
    size_t cacert_pem_len;
    bool use_global_ca_store;
    bool skip_common_name;
} esp_tls_cfg_t;

static inline esp_tls_t *esp_tls_init(void) { return NULL; }

static inline esp_err_t esp_tls_set_conn_sockfd(esp_tls_t *tls, int fd) {
    (void)tls; (void)fd;
    return ESP_FAIL;
}

static inline esp_err_t esp_tls_set_conn_state(esp_tls_t *tls, int state) {
    (void)tls; (void)state;
    return ESP_FAIL;
}

static inline esp_err_t esp_tls_conn_new_sync(const char *host, int port,
    int timeout_ms, const esp_tls_cfg_t *cfg, esp_tls_t *tls) {
    (void)host; (void)port; (void)timeout_ms; (void)cfg; (void)tls;
    return ESP_FAIL;
}

static inline void esp_tls_conn_destroy(esp_tls_t *tls) { (void)tls; }

static inline int esp_tls_conn_write(esp_tls_t *tls, const char *data, int len) {
    (void)tls; (void)data; (void)len;
    return -1;
}

static inline int esp_tls_conn_read(esp_tls_t *tls, char *data, int len) {
    (void)tls; (void)data; (void)len;
    return -1;
}
