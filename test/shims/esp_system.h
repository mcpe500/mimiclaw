#pragma once
#include "esp_err.h"

static inline void ESP_ERROR_CHECK(esp_err_t err) { if (err != ESP_OK) { exit(1); } }
static inline void esp_restart(void) { exit(0); }
