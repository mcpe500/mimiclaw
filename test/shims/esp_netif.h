#pragma once
#include "esp_err.h"

typedef void *esp_netif_t;

static inline esp_netif_t esp_netif_create_default_wifi_sta() { return (esp_netif_t)1; }
static inline esp_netif_t esp_netif_create_default_wifi_ap() { return (esp_netif_t)1; }
