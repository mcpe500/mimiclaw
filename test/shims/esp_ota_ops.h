#pragma once
#include "esp_err.h"

static inline esp_err_t esp_ota_begin(void *data, size_t size) { return ESP_OK; }
static inline esp_err_t esp_ota_end(void) { return ESP_OK; }
static inline esp_err_t esp_ota_set_boot_partition(void) { return ESP_OK; }
static inline esp_err_t esp_ota_write(const void *data, size_t size) { return ESP_OK; }
