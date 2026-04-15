#pragma once
#include "esp_err.h"
#include <stddef.h>

typedef int nvs_handle_t;

#define NVS_READWRITE 1
#define NVS_READONLY  0

static inline esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle) { *handle = 1; return ESP_OK; }
static inline void nvs_close(nvs_handle_t handle) { (void)handle; }
static inline esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) { (void)handle; (void)key; return ESP_OK; }
static inline esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *buf, size_t *len) { (void)handle; (void)key; (void)buf; (void)len; return ESP_OK; }
static inline esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *val) { (void)handle; (void)key; (void)val; return ESP_OK; }
static inline esp_err_t nvs_commit(nvs_handle_t handle) { (void)handle; return ESP_OK; }
