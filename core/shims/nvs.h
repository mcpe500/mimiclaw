#pragma once
#include <stddef.h>
#include "esp_err.h"

typedef int nvs_handle_t;

#define NVS_READWRITE 1
#define NVS_READONLY  0

esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *buf, size_t *len);
esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *val);
esp_err_t nvs_commit(nvs_handle_t handle);
