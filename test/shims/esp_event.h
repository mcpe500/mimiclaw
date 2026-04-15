#pragma once
#include "esp_err.h"

typedef int esp_event_base_t;
typedef int esp_event_handle_t;

static inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
static inline esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int flags, esp_event_handle_t handler, void *arg, void *instance) { return ESP_OK; }
