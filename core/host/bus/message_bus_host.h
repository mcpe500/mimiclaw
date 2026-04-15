#pragma once
#include "esp_err.h"
#include <stddef.h>

esp_err_t message_bus_init(void);
esp_err_t message_bus_push_inbound(const void *msg);
esp_err_t message_bus_pop_inbound(void *msg, uint32_t timeout_ms);
esp_err_t message_bus_push_outbound(const void *msg);
esp_err_t message_bus_pop_outbound(void *msg, uint32_t timeout_ms);
void message_bus_deinit(void);
