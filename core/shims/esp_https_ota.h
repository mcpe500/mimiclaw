#pragma once
#include "esp_err.h"
#include <stddef.h>

typedef struct {
    const char *url;
} esp_https_ota_config_t;

static inline esp_err_t esp_https_ota(const esp_https_ota_config_t *config) {
    (void)config;
    return ESP_ERR_NOT_SUPPORTED;
}
