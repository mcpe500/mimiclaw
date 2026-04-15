#pragma once
#include "esp_err.h"

#define ESP_ERR_NVS_NO_FREE_PAGES 0x1001
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1002

static inline esp_err_t nvs_flash_init(void) { return ESP_OK; }
static inline esp_err_t nvs_flash_erase(void) { return ESP_OK; }
