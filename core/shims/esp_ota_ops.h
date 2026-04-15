#pragma once
#include "esp_err.h"

static inline esp_err_t esp_ota_begin(void *partition, size_t image_size, void *handle) {
    (void)partition; (void)image_size; (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t esp_ota_end(void *handle) {
    (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
}
